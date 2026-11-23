#include "anenji.h"
#include <algorithm>
#include <cmath>
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

void AnenjiManager::setup() {
  if (direction_ != nullptr) direction_->turn_off();
  next_scan_ = scan_start_;
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
  phase_ = next;
  request_started_ = millis();
}

void AnenjiManager::complete_request_(bool success) {
  if (!success && phase_ == Phase::WAIT_SERIAL)
    ESP_LOGW(TAG, "Serial discovery failed at address %u (received %u bytes)", active_address_, unsigned(rx_.size()));
  phase_ = Phase::IDLE;
  rx_.clear();
}

void AnenjiManager::process_frame_() {
  if (rx_.size() < 5) return;
  if (rx_[0] != active_address_ || rx_[1] != 0x03) {
    ESP_LOGW(TAG, "Unexpected response: requested=%u received=%u function=0x%02X", active_address_, rx_[0], rx_[1]);
    complete_request_(false);
    return;
  }
  size_t expected = size_t(rx_[2]) + 5;
  if (expected > 128) { complete_request_(false); return; }
  if (rx_.size() < expected) return;
  uint16_t received_crc = uint16_t(rx_[expected - 2]) | (uint16_t(rx_[expected - 1]) << 8);
  if (crc16_(rx_.data(), expected - 2) != received_crc) {
    ESP_LOGW(TAG, "CRC error from address %u", active_address_);
    complete_request_(false);
    return;
  }
  if (phase_ == Phase::WAIT_SERIAL && rx_[2] == SERIAL_COUNT * 2) {
    std::string serial;
    for (size_t i = 3; i < 3 + SERIAL_COUNT * 2; i++) {
      char c = char(rx_[i]);
      if (c >= 32 && c <= 126) serial.push_back(c);
    }
    ESP_LOGI(TAG, "Address %u serial response: %s", active_address_, serial.c_str());
    while (!serial.empty() && serial.back() == ' ') serial.pop_back();
    if (!serial.empty()) {
      auto it = std::find_if(devices_.begin(), devices_.end(),
                             [&](const Inverter &d) { return d.serial == serial; });
      if (it == devices_.end() && devices_.size() < max_inverters_) {
        devices_.push_back({serial, active_address_, NAN, millis()});
        ESP_LOGI(TAG, "Discovered inverter serial=%s address=%u", serial.c_str(), active_address_);
      } else if (it != devices_.end()) {
        it->address = active_address_;
        it->last_seen = millis();
      }
    }
  } else if (phase_ == Phase::WAIT_VOLTAGE && rx_[2] == 2) {
    float voltage = float((uint16_t(rx_[3]) << 8) | rx_[4]) / 10.0f;
    for (auto &d : devices_) {
      if (d.address == active_address_) {
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
  if (discovering_) {
    if (next_scan_ <= scan_end_) {
      ESP_LOGI(TAG, "Scanning ANENJI Modbus address %u", next_scan_);
      send_read_(next_scan_++, SERIAL_REG, SERIAL_COUNT, Phase::WAIT_SERIAL);
      return;
    }
    discovering_ = false;
    last_discovery_ = millis();
    ESP_LOGI(TAG, "Discovery complete: %u serial-identified inverters", unsigned(devices_.size()));
    for (const auto &d : devices_) ESP_LOGI(TAG, "  Address %u serial %s", d.address, d.serial.c_str());
    poll_index_ = 0;
  }
  if (devices_.empty()) {
    if (millis() - last_discovery_ >= 30000) {
      next_scan_ = scan_start_;
      discovering_ = true;
    }
    return;
  }
  if (poll_index_ < devices_.size()) {
    auto &device = devices_[poll_index_++];
    send_read_(device.address, VOLTAGE_REG, 1, Phase::WAIT_VOLTAGE);
    return;
  }
  if (millis() - last_cycle_ >= poll_ms_) {
    poll_index_ = 0;
    last_cycle_ = millis();
  }
  if (millis() - last_discovery_ >= (devices_.size() < max_inverters_ ? 30000UL : 300000UL)) {
    next_scan_ = scan_start_;
    discovering_ = true;
  }
}

void AnenjiManager::loop() {
  if (phase_ != Phase::IDLE) {
    while (available()) {
      uint8_t byte;
      if (read_byte(&byte)) rx_.push_back(byte);
      if (rx_.size() > 128) { complete_request_(false); break; }
    }
    if (phase_ != Phase::IDLE) {
      process_frame_();
      if (phase_ != Phase::IDLE && millis() - request_started_ >= RESPONSE_TIMEOUT_MS) {
        ESP_LOGW(TAG, "Modbus timeout address=%u phase=%u bytes=%u", active_address_, unsigned(phase_), unsigned(rx_.size()));
        complete_request_(false);
      }
    }
  }
  if (phase_ == Phase::IDLE) start_next_();
}

void AnenjiManager::update() {
  // Polling is scheduled by the non-blocking loop() state machine.
}

}  // namespace anenji
}  // namespace esphome
