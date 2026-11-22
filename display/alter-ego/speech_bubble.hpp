#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "hg/canvas.hpp"
#include "hg/font.hpp"

namespace alter_ego {

struct ClockTime {
  int hour;
  int minute;
};

enum class WifiStatus { Offline, Connecting, Connected, Hotspot };

class SpeechBubble {
 public:
  SpeechBubble() { show_message("Hi, Felix!"); }

  void begin_startup(uint32_t now) {
    startup_ = Startup::Greeting;
    startup_at_ = now;
  }

  // Shows new text immediately and returns to it after cycling through the views.
  void show_message(std::string_view message) {
    startup_ = Startup::Done;
    view_ = View::Text;
    message_.assign(message);
    set_text(message_);
  }

  void touch(bool down, int x, int y, uint32_t now) {
    if (down && !held_) {
      start_x_ = x;
      start_y_ = y;
      pressed_at_ = now;
      moved_ = false;
    }
    if (down && (std::abs(x - start_x_) > 14 || std::abs(y - start_y_) > 14)) moved_ = true;
    // The touchscreen may report zero coordinates on lift.
    if (!down && held_ && !moved_ && now - pressed_at_ <= 350) {
      if (startup_ != Startup::Done) view_ = View::Text;
      startup_ = Startup::Done;
      switch (view_) {
        case View::Text: view_ = View::Clock; next_update_ = now; break;
        case View::Clock: view_ = View::Wifi; set_text(wifi_text(wifi_)); break;
        case View::Wifi: view_ = View::Text; set_text(message_); break;
      }
    }
    held_ = down;
  }

  template <typename ReadClock>
  void update(uint32_t now, ReadClock read_clock, WifiStatus wifi) {
    if (startup_ == Startup::Done && wifi != wifi_) view_ = View::Wifi;
    wifi_ = wifi;
    if (startup_ == Startup::Greeting && now - startup_at_ >= 1500) {
      startup_ = Startup::Network;
      startup_at_ = now;
      view_ = View::Wifi;
    }
    if (startup_ == Startup::Network && online(wifi)) {
      startup_ = Startup::Connected;
      startup_at_ = now;
    } else if (startup_ == Startup::Connected && !online(wifi)) {
      startup_ = Startup::Network;
    } else if (startup_ == Startup::Connected && now - startup_at_ >= 1200) {
      startup_ = Startup::Done;
      view_ = View::Text;
      set_text(message_);
    }
    if (view_ == View::Wifi) set_text(wifi_text(wifi));
    if (view_ != View::Clock || static_cast<int32_t>(now - next_update_) < 0) return;
    next_update_ = now + 1000;
    const auto time = read_clock();
    if (time && time->hour >= 0 && time->hour < 24 && time->minute >= 0 && time->minute < 60) {
      char text[6];
      std::snprintf(text, sizeof(text), "%02u:%02u",
                    static_cast<unsigned>(time->hour), static_cast<unsigned>(time->minute));
      set_text(text);
    } else {
      set_text("Uhr nicht gestellt");
    }
  }

  void draw(hg::Canvas& canvas) const {
    constexpr auto border = hg::rgb565(222, 161, 81);
    constexpr auto fill = hg::rgb565(19, 16, 27);
    constexpr auto ink = hg::rgb565(255, 231, 185);
    constexpr int scale = 2;
    constexpr int top = 318;
    constexpr int center = 233;
    const int height = 22 + static_cast<int>(lines_.size()) * hg::Canvas::line_height(scale);
    int width = 132;
    for (const auto& line : lines_) width = std::max(width, 24 + hg::Canvas::text_width(line, scale));
    const int left = center - width / 2;

    for (int row = 0; row < 17; ++row) {
      const int half = row / 2;
      canvas.fill_rect(center - half, top - 16 + row, half * 2 + 1, 1, border);
    }
    canvas.fill_round_rect(left, top, width, height, 13, border);
    canvas.fill_round_rect(left + 2, top + 2, width - 4, height - 4, 11, fill);
    for (int row = 0; row < 15; ++row) {
      const int half = row / 2;
      canvas.fill_rect(center - half, top - 12 + row, half * 2 + 1, 1, fill);
    }
    int y = top + 12;
    for (const auto& line : lines_) {
      canvas.text(center - hg::Canvas::text_width(line, scale) / 2, y, line, scale, ink);
      y += hg::Canvas::line_height(scale);
    }
  }

 private:
  enum class View { Text, Clock, Wifi };
  enum class Startup { Done, Greeting, Network, Connected };

  static bool online(WifiStatus wifi) {
    return wifi == WifiStatus::Connected || wifi == WifiStatus::Hotspot;
  }

  static std::string_view wifi_text(WifiStatus wifi) {
    switch (wifi) {
      case WifiStatus::Connecting: return "WLAN: Verbinde ...";
      case WifiStatus::Connected: return "Heim-WLAN aktiv";
      case WifiStatus::Hotspot: return "iPhone-Hotspot aktiv";
      case WifiStatus::Offline: return "WLAN: Offline";
    }
    return "WLAN: Offline";
  }

  void set_text(std::string_view message) {
    const auto bounded = message.substr(0, hg::font::prefix(message, 64));
    const bool clipped = bounded.size() < message.size();
    if (bounded == rendered_text_ && clipped == clipped_) return;
    rendered_text_ = bounded;
    clipped_ = clipped;
    lines_ = hg::wrap_text(rendered_text_, 19);
    const bool overflow = lines_.size() > 2;
    if (overflow) lines_.resize(2);
    if ((overflow || clipped_) && !lines_.empty()) {
      auto& last = lines_.back();
      last.resize(hg::font::prefix(last, 16));
      last += "...";
    }
  }

  std::string message_;
  std::string rendered_text_;
  std::vector<std::string> lines_;
  uint32_t pressed_at_ = 0;
  uint32_t next_update_ = 0;
  int start_x_ = 0;
  int start_y_ = 0;
  bool clipped_ = false;
  bool held_ = false;
  bool moved_ = false;
  View view_ = View::Text;
  Startup startup_ = Startup::Done;
  WifiStatus wifi_ = WifiStatus::Offline;
  uint32_t startup_at_ = 0;
};

}  // namespace alter_ego
