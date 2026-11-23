#pragma once

#include <array>
#include <string>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/output/binary_output.h"

namespace esphome {
namespace anenji {

class AnenjiManager : public PollingComponent, public uart::UARTDevice {
 public:
  void set_direction_output(output::BinaryOutput *out) { direction_ = out; }
  void set_scan_start(uint8_t value) { scan_start_ = value; }
  void set_scan_end(uint8_t value) { scan_end_ = value; }
  void set_max_inverters(uint8_t value) { max_inverters_ = value; }
  void set_poll_ms(uint32_t value) { poll_ms_ = value; }
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

 protected:
  struct Inverter {
    std::string serial;
    uint8_t address{0};
    float battery_voltage{NAN};
    uint32_t last_seen{0};
  };
  enum class Phase : uint8_t { IDLE, WAIT_PROBE, SERIAL_GAP, WAIT_SERIAL, WAIT_VOLTAGE };
  static constexpr uint16_t SERIAL_REG = 51014;
  static constexpr uint8_t SERIAL_COUNT = 25;
  static constexpr uint16_t VOLTAGE_REG = 30000;
  static constexpr uint32_t RESPONSE_TIMEOUT_MS = 350;
  output::BinaryOutput *direction_{nullptr};
  std::vector<Inverter> devices_;
  std::vector<uint8_t> rx_;
  Phase phase_{Phase::IDLE};
  uint8_t scan_start_{1}, scan_end_{6}, max_inverters_{6}, next_scan_{1};
  uint8_t active_address_{0};
  uint32_t poll_ms_{5000}, request_started_{0}, last_cycle_{0}, last_discovery_{0};
  bool discovering_{true};
  size_t poll_index_{0};
  std::string pending_serial_;
  uint8_t scan_retries_{0};
  uint32_t serial_ready_at_{0};

  static uint16_t crc16_(const uint8_t *data, size_t size);
  void send_read_(uint8_t address, uint16_t reg, uint16_t count, Phase next);
  void complete_request_(bool success);
  void process_frame_();
  void start_next_();
};

}  // namespace anenji
}  // namespace esphome
