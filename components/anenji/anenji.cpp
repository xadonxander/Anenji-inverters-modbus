#include "anenji.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "esphome/core/hal.h"

namespace esphome {
namespace anenji {

static const char *const TAG = "anenji";

uint16_t AnenjiManager::crc16_(const uint8_t *data, size_t size) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

namespace {
struct ReadBlock { uint16_t reg; uint8_t count; };
constexpr ReadBlock BLOCKS[] = {{30000,6},{30024,1},{30046,4},{30058,3},{30068,3},{30071,5},{52044,16},{52071,2}};
struct Metric { uint8_t block, offset; int16_t divisor; };
constexpr Metric METRICS[] = {{0,0,10},{0,1,10},{0,2,1},{1,0,10},{2,3,1},{3,0,10},{3,1,-10},{4,0,-1},{5,0,10},{5,2,-10},{5,3,-10},{5,4,-10},{6,0,10},{6,2,10},{6,10,10},{6,12,10},{7,0,10}};
}
uint8_t AnenjiManager::bind_(const std::string &serial) {
  for (uint8_t i=0;i<max_inverters_;i++) if (serial == bindings_[i].serial) return i;
  for (uint8_t i=0;i<max_inverters_;i++) {
    if (bindings_[i].serial[0]) continue;
    if (serial.size() >= sizeof(bindings_[i].serial)) return 255;
    std::memcpy(bindings_[i].serial, serial.c_str(), serial.size()+1);
    if (!prefs_[i].save(&bindings_[i])) {
      bindings_[i].serial[0]=0;
      return 255;
    }
    global_preferences->sync();
    return i;
  }
  return 255;
}
void AnenjiManager::invalidate_(uint8_t slot, uint8_t block) {
  if (slot >= 6) return;
  for (size_t i=0;i<sizeof(METRICS)/sizeof(METRICS[0]);i++)
    if (METRICS[i].block == block && metrics_[slot][i] && !std::isnan(metrics_[slot][i]->state)) metrics_[slot][i]->publish_state(NAN);
}
void AnenjiManager::setup() {
  for (uint8_t i=0;i<max_inverters_;i++) {
    prefs_[i]=global_preferences->make_preference<Binding>(0xA6E10000UL+i);
    prefs_[i].load(&bindings_[i]);
    bindings_[i].serial[50]=0;
    if (serial_sensors_[i] && bindings_[i].serial[0]) serial_sensors_[i]->publish_state(bindings_[i].serial);
  }
  if (direction_ != nullptr) direction_->turn_off();
  next_scan_ = scan_start_;
  last_bus_activity_ = millis();
  discovering_ = true;
  last_cycle_ = millis() - poll_ms_;
  ESP_LOGI(TAG, "Starting ANENJI serial discovery on addresses %u-%u", scan_start_, scan_end_);
}

void AnenjiManager::dump_config() {
  ESP_LOGCONFIG(TAG, "ANENJI RS485 manager (read-only)");
  ESP_LOGCONFIG(TAG, "  Discovery: %u-%u, maximum %u", scan_start_, scan_end_, max_inverters_);
  ESP_LOGCONFIG(TAG, "  Poll interval: %u ms", poll_ms_);
  LOG_UPDATE_INTERVAL(this);
}

void AnenjiManager::send_read_(uint8_t address, uint16_t reg, uint16_t count, Phase next) {
  uint8_t request[8] = {address, 0x03, uint8_t(reg >> 8), uint8_t(reg), uint8_t(count >> 8), uint8_t(count), 0, 0};
  uint16_t crc = crc16_(request, 6);
  request[6] = uint8_t(crc);
  request[7] = uint8_t(crc >> 8);
  while (available()) read();
  rx_.clear();
  if (direction_ != nullptr) direction_->turn_on();
  delayMicroseconds(2000);
  write_array(request, sizeof(request));
  flush();
  delayMicroseconds(175);
  if (direction_ != nullptr) direction_->turn_off();
  active_address_ = address;
  expected_bytes_ = uint8_t(count * 2);
  phase_ = next;
  request_started_ = millis();
}

void AnenjiManager::complete_request_(bool success) {
  if (!success && (phase_ == Phase::WAIT_SERIAL || phase_ == Phase::WAIT_PROBE))
    ESP_LOGW(TAG, "Serial discovery failed at address %u (received %u bytes)", active_address_, unsigned(rx_.size()));
  if (!success && phase_ == Phase::WAIT_VOLTAGE) invalidate_(active_slot_, block_ == 0 ? 7 : block_-1);
  phase_ = Phase::IDLE;
  last_bus_activity_ = millis();
  rx_.clear();
}

void AnenjiManager::process_frame_() {
  const size_t expected = size_t(expected_bytes_) + 5;
  // Search for the complete CRC-valid response; do not abort on stray bytes.
  size_t offset = 0;
  bool matched = false;
  for (; offset + expected <= rx_.size(); ++offset) {
    const auto *p = rx_.data() + offset;
    if (p[0] != active_address_ || p[1] != 0x03 || p[2] != expected_bytes_) continue;
    const uint16_t crc = uint16_t(p[expected-2]) | (uint16_t(p[expected-1]) << 8);
    if (crc16_(p, expected-2) == crc) { matched = true; break; }
  }
  if (!matched) return;
  if (offset) {
    ESP_LOGW(TAG, "Discarding %u unrelated RX bytes before valid response", unsigned(offset));
    rx_.erase(rx_.begin(), rx_.begin() + offset);
  }
  if (phase_ == Phase::WAIT_PROBE && rx_[2] == 2) {
    const float volts = float((uint16_t(rx_[3]) << 8) | rx_[4]) / 10.0f;
    ESP_LOGI(TAG, "Address %u responds: battery=%.1f V; requesting serial", active_address_, volts);
    const uint8_t address = active_address_;
    complete_request_(true);
    // Allow the inverter to finish its previous transaction before the long serial read.
    active_address_ = address;
    serial_ready_at_ = millis() + 25;
    phase_ = Phase::SERIAL_GAP;
    return;
  } else if (phase_ == Phase::WAIT_SERIAL && rx_[2] == SERIAL_COUNT * 2) {
    std::string serial;
    for (size_t i = 3; i < 3 + SERIAL_COUNT * 2; i++) {
      char c = char(rx_[i]);
      if (c >= 32 && c <= 126) serial.push_back(c);
    }
    ESP_LOGI(TAG, "Address %u serial response: %s", active_address_, serial.c_str());
    while (!serial.empty() && serial.back() == ' ') serial.pop_back();
    if (!serial.empty()) {
      const uint8_t slot = bind_(serial);
      if (slot == 255) { complete_request_(true); return; }
      if (serial_sensors_[slot]) serial_sensors_[slot]->publish_state(serial);
      if (address_sensors_[slot]) address_sensors_[slot]->publish_state(active_address_);
      auto it = std::find_if(devices_.begin(), devices_.end(),
                             [&](const Inverter &d) { return d.serial == serial; });
      if (it == devices_.end() && devices_.size() < max_inverters_) {
        devices_.push_back({serial, active_address_, NAN, millis(), slot});
        ESP_LOGI(TAG, "Discovered inverter serial=%s address=%u", serial.c_str(), active_address_);
      } else if (it != devices_.end()) {
        it->address = active_address_;
        it->last_seen = millis();
      }
    }
  } else if (phase_ == Phase::WAIT_VOLTAGE) {
    const uint8_t completed_block = block_ == 0 ? 7 : block_-1;
    for (size_t i=0;i<sizeof(METRICS)/sizeof(METRICS[0]);i++) {
      const auto &m=METRICS[i];
      if (m.block != completed_block || active_slot_ >= 6 || !metrics_[active_slot_][i]) continue;
      const size_t pos=3+2*m.offset;
      const uint16_t raw=(uint16_t(rx_[pos])<<8)|rx_[pos+1];
      const float value=m.divisor<0 ? float(int16_t(raw)) / -m.divisor : float(raw)/m.divisor;
      metrics_[active_slot_][i]->publish_state(value);
    }
    float voltage = float((uint16_t(rx_[3]) << 8) | rx_[4]) / 10.0f;
    for (auto &d : devices_) {
      if (d.slot == active_slot_ && completed_block == 0) {
        d.battery_voltage = voltage;
        d.last_seen = millis();
        ESP_LOGD(TAG, "serial=%s battery=%.1f V (address=%u)", d.serial.c_str(), voltage, active_address_);
        break;
      }
    }
  } else {
    ESP_LOGW(TAG, "Unexpected byte count %u from address %u during phase %u", rx_[2], active_address_, unsigned(phase_));
  }
  complete_request_(true);
}

void AnenjiManager::start_next_() {
  const uint32_t now = millis();
  // A rescan must preempt normal polling, even when devices are already known.
  if (!discovering_ &&
      now - last_discovery_ >= (devices_.size() < max_inverters_ ? 30000UL : 300000UL)) {
    discovering_ = true;
    next_scan_ = scan_start_;
    devices_.clear();
    block_ = 0;
    ESP_LOGI(TAG, "Starting periodic ANENJI address scan %u-%u", scan_start_, scan_end_);
  }
  if (discovering_) {
    if (next_scan_ <= scan_end_) {
      const uint8_t address = next_scan_++;
      ESP_LOGI(TAG, "PROBE address=%u register=%u", address, VOLTAGE_REG);
      send_read_(address, VOLTAGE_REG, 1, Phase::WAIT_PROBE);
      return;
    }
    discovering_ = false;
    last_discovery_ = now;
    // Avoid a duplicate immediate polling cycle after discovery.
    last_cycle_ = now;
    poll_index_ = devices_.size();
    block_ = 0;
    for (uint8_t i=0;i<max_inverters_;i++) {
      if (bindings_[i].serial[0] && std::none_of(devices_.begin(), devices_.end(), [i](const Inverter &d){return d.slot==i;})) {
        if (address_sensors_[i] && !std::isnan(address_sensors_[i]->state)) address_sensors_[i]->publish_state(NAN);
        for (uint8_t j=0;j<8;j++) invalidate_(i,j);
      }
    }
    ESP_LOGI(TAG, "Discovery complete: %u serial-identified inverters", unsigned(devices_.size()));
    for (const auto &d : devices_)
      ESP_LOGI(TAG, "  address=%u serial=%s", d.address, d.serial.c_str());
  }
  if (devices_.empty()) return;
  // Start a new polling cycle only when the configured interval has elapsed.
  if (poll_index_ >= devices_.size()) {
    if (now - last_cycle_ < poll_ms_) return;
    poll_index_ = 0;
    last_cycle_ = now;
  }
  const auto &device = devices_[poll_index_];
  active_slot_ = device.slot;
  const auto &read = BLOCKS[block_];
  if (++block_ >= sizeof(BLOCKS)/sizeof(BLOCKS[0])) { block_=0; ++poll_index_; }
  send_read_(device.address, read.reg, read.count, Phase::WAIT_VOLTAGE);
}

void AnenjiManager::loop() {
  if (phase_ == Phase::IDLE || phase_ == Phase::SERIAL_GAP) {
    uint8_t discarded;
    while (available() && read_byte(&discarded)) last_bus_activity_ = millis();
  }
  if (phase_ == Phase::SERIAL_GAP) {
    if (int32_t(millis() - serial_ready_at_) >= 0 && millis() - last_bus_activity_ >= 25) {
      ESP_LOGI(TAG, "Reading serial at address %u after turnaround gap", active_address_);
      send_read_(active_address_, SERIAL_REG, SERIAL_COUNT, Phase::WAIT_SERIAL);
    }
    return;
  }
  if (phase_ != Phase::IDLE) {
    while (available()) {
      uint8_t byte;
      if (read_byte(&byte)) rx_.push_back(byte);
      if (rx_.size() > 256) rx_.erase(rx_.begin(), rx_.begin() + 128);
    }
    if (phase_ != Phase::IDLE) {
      process_frame_();
      if (phase_ != Phase::IDLE && millis() - request_started_ >= RESPONSE_TIMEOUT_MS) {
        ESP_LOGW(TAG, "Modbus timeout address=%u phase=%u bytes=%u", active_address_, unsigned(phase_), unsigned(rx_.size()));
        complete_request_(false);
      }
    }
  }
  // Enforce the original scanner's approximately 70–80ms address spacing.
  if (phase_ == Phase::IDLE && millis() - last_bus_activity_ >= 80) start_next_();
}

void AnenjiManager::update() {
  // Polling is scheduled by the non-blocking loop() state machine.
}

}  // namespace anenji
}  // namespace esphome
