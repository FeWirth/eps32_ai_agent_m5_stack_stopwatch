#pragma once

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <optional>

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "speech_bubble.hpp"

namespace alter_ego {

// Keep Berlin time from SNTP when available, with the RX8130CE as the offline source.
class StopWatchClock {
 public:
  explicit StopWatchClock(i2c_master_bus_handle_t bus) {
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    if (!bus) return;
    i2c_device_config_t config{};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = 0x32;
    config.scl_speed_hz = 400000;
    if (i2c_master_bus_add_device(bus, &config, &device_) != ESP_OK) device_ = nullptr;
    enable_backup();
  }

  ~StopWatchClock() {
    if (sntp_started_) esp_netif_sntp_deinit();
    if (device_) i2c_master_bus_rm_device(device_);
  }

  StopWatchClock(const StopWatchClock&) = delete;
  StopWatchClock& operator=(const StopWatchClock&) = delete;

  void tick_network(bool connected) {
    if (connected && !connected_) {
      if (sntp_started_) {
        if (esp_netif_sntp_start() != ESP_OK) ESP_LOGW("hg.rtc", "could not restart SNTP");
      } else {
        const esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        sntp_started_ = esp_netif_sntp_init(&config) == ESP_OK;
        if (!sntp_started_) ESP_LOGW("hg.rtc", "could not start SNTP");
      }
    }
    connected_ = connected;
    if (!sntp_started_ || esp_netif_sntp_sync_wait(0) != ESP_OK) return;

    const auto timestamp = std::time(nullptr);
    std::tm local{};
    if (!localtime_r(&timestamp, &local) || local.tm_year < 125) return;
    network_time_ = true;
    const bool saved = set(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_wday,
                           local.tm_hour, local.tm_min, local.tm_sec);
    ESP_LOGI("hg.rtc", "SNTP Berlin %02d:%02d:%02d; RTC %s", local.tm_hour, local.tm_min,
             local.tm_sec, saved ? "verified" : "write failed");
  }

  std::optional<ClockTime> read() const {
    if (network_time_) {
      const auto timestamp = std::time(nullptr);
      std::tm local{};
      if (localtime_r(&timestamp, &local)) return ClockTime{local.tm_hour, local.tm_min};
    }
    return read_rtc();
  }

 private:
  std::optional<ClockTime> read_rtc() const {
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

 public:
  std::string command(const char* line) const {
    if (std::string_view(line) == "rtc") {
      char reply[24];
      const auto time = read_rtc();
      if (!time) return "@rtc unset";
      std::snprintf(reply, sizeof(reply), "@rtc %02d:%02d", time->hour, time->minute);
      return reply;
    }
    int year, month, day, weekday, hour, minute, second;
    char extra;
    if (std::sscanf(line, "rtc %4d %2d %2d %1d %2d %2d %2d %c",
                    &year, &month, &day, &weekday, &hour, &minute, &second, &extra) != 7) return "@rtc error";
    return set(year, month, day, weekday, hour, minute, second) ? "@rtc ok" : "@rtc error";
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
    const auto time = read_rtc();
    return time && time->hour == hour && time->minute == minute;
  }

  bool connected_ = false;
  bool sntp_started_ = false;
  bool network_time_ = false;
  i2c_master_dev_handle_t device_ = nullptr;
};

}  // namespace alter_ego
