#pragma once

#include <cstdio>
#include <ctime>
#include <optional>

#include "driver/i2c_master.h"
#include "driver/usb_serial_jtag.h"
#include "speech_bubble.hpp"

namespace alter_ego {

// Read the time already stored in the board's RX8130CE on the shared I2C bus.
class StopWatchClock {
 public:
  explicit StopWatchClock(i2c_master_bus_handle_t bus) {
    if (!bus) return;
    i2c_device_config_t config{};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = 0x32;
    config.scl_speed_hz = 400000;
    if (i2c_master_bus_add_device(bus, &config, &device_) != ESP_OK) device_ = nullptr;
    enable_backup();
  }

  ~StopWatchClock() {
    if (usb_ready_) usb_serial_jtag_driver_uninstall();
    if (device_) i2c_master_bus_rm_device(device_);
  }

  StopWatchClock(const StopWatchClock&) = delete;
  StopWatchClock& operator=(const StopWatchClock&) = delete;

  void enable_usb() {
    usb_serial_jtag_driver_config_t config{256, 256};
    usb_ready_ = usb_serial_jtag_driver_install(&config) == ESP_OK;
  }

  void poll_usb() {
    if (!usb_ready_) return;
    uint8_t buffer[64];
    const int count = usb_serial_jtag_read_bytes(buffer, sizeof(buffer), 0);
    for (int i = 0; i < count; ++i) {
      const char ch = static_cast<char>(buffer[i]);
      if (ch == '\r' || ch == '\n') {
        if (length_ && length_ < sizeof(command_)) {
          command_[length_] = '\0';
          handle_command();
        }
        length_ = 0;
      } else if (length_ < sizeof(command_)) {
        command_[length_++] = ch;
      }
    }
  }

  std::optional<ClockTime> read() const {
    constexpr uint8_t first_register = 0x10;
    constexpr int flags = 0x1d - first_register;
    constexpr int control = 0x1e - first_register;
    uint8_t registers[control + 1]{};
    if (!device_ || i2c_master_transmit_receive(device_, &first_register, 1, registers,
                                               sizeof(registers), 20) != ESP_OK ||
        (registers[flags] & 0x02) || (registers[control] & 0x40)) return std::nullopt;
    const uint8_t hour = registers[2] & 0x3f;
    const uint8_t minute = registers[1] & 0x7f;
    if ((hour & 0xf) > 9 || (minute & 0xf) > 9 ||
        hour > 0x23 || minute > 0x59) return std::nullopt;
    return ClockTime{(hour >> 4) * 10 + (hour & 0xf), (minute >> 4) * 10 + (minute & 0xf)};
  }

 private:
  bool enable_backup() const {
    // StopWatch has a rechargeable RTC backup supply. Enable switchover and charging.
    constexpr uint8_t control1 = 0x1f;
    constexpr uint8_t backup = 0x30;  // CHGEN | INIEN
    uint8_t value = 0;
    if (!device_ || i2c_master_transmit_receive(device_, &control1, 1, &value, 1, 20) != ESP_OK) return false;
    if ((value & backup) == backup) return true;
    const uint8_t settings[] = {control1, static_cast<uint8_t>(value | backup)};
    return i2c_master_transmit(device_, settings, sizeof(settings), 20) == ESP_OK;
  }

  void handle_command() {
    if (std::string_view(command_) == "rtc") {
      char reply[24];
      const auto time = read();
      const int length = time
          ? std::snprintf(reply, sizeof(reply), "@rtc %02d:%02d\r\n", time->hour, time->minute)
          : std::snprintf(reply, sizeof(reply), "@rtc unset\r\n");
      usb_serial_jtag_write_bytes(reply, length, pdMS_TO_TICKS(20));
      return;
    }
    int year, month, day, weekday, hour, minute, second;
    char extra;
    if (std::sscanf(command_, "rtc %4d %2d %2d %1d %2d %2d %2d %c",
                    &year, &month, &day, &weekday, &hour, &minute, &second, &extra) != 7) return;
    const bool ok = set(year, month, day, weekday, hour, minute, second);
    const char* reply = ok ? "@rtc ok\r\n" : "@rtc error\r\n";
    usb_serial_jtag_write_bytes(reply, ok ? 9 : 12, pdMS_TO_TICKS(20));
  }

  bool set(int year, int month, int day, int weekday, int hour, int minute, int second) const {
    if (!device_ || year < 2000 || year > 2099 || month < 1 || month > 12 ||
        day < 1 || day > 31 || weekday < 0 || weekday > 6 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return false;
    std::tm date{};
    date.tm_year = year - 1900;
    date.tm_mon = month - 1;
    date.tm_mday = day;
    if (std::mktime(&date) == std::time_t{-1} || date.tm_mday != day || date.tm_wday != weekday) return false;

    if (!enable_backup()) return false;
    constexpr uint8_t flag_reg = 0x1d;
    constexpr uint8_t control_reg = 0x1e;
    constexpr uint8_t stop = 0x40;
    uint8_t status[2]{};
    if (i2c_master_transmit_receive(device_, &flag_reg, 1, status, sizeof(status), 20) != ESP_OK) return false;
    const uint8_t pause[] = {control_reg, static_cast<uint8_t>(status[1] | stop)};
    if (i2c_master_transmit(device_, pause, sizeof(pause), 20) != ESP_OK) return false;
    const auto bcd = [](int value) { return static_cast<uint8_t>((value / 10) * 16 + value % 10); };
    const uint8_t calendar[] = {0x10, bcd(second), bcd(minute), bcd(hour),
                                static_cast<uint8_t>(1u << weekday), bcd(day), bcd(month), bcd(year % 100)};
    const bool written = i2c_master_transmit(device_, calendar, sizeof(calendar), 20) == ESP_OK;
    const uint8_t resume[] = {control_reg, static_cast<uint8_t>(status[1] & ~stop)};
    if (i2c_master_transmit(device_, resume, sizeof(resume), 20) != ESP_OK || !written) return false;
    const uint8_t flags[] = {flag_reg, static_cast<uint8_t>(status[0] & ~0x02)};
    if (i2c_master_transmit(device_, flags, sizeof(flags), 20) != ESP_OK) return false;
    const auto time = read();
    return time && time->hour == hour && time->minute == minute;
  }

  bool usb_ready_ = false;
  size_t length_ = 0;
  char command_[40]{};
  i2c_master_dev_handle_t device_ = nullptr;
};

}  // namespace alter_ego
