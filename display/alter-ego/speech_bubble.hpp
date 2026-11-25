#pragma once

#include <algorithm>
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

  bool touch(bool down, int x, int y, uint32_t now) {
    if (down && !held_) {
      start_x_ = x;
      start_y_ = y;
      pressed_at_ = now;
      moved_ = false;
      scrolling_ = false;
      const auto box = layout();
      scroll_candidate_ = max_scroll() > 0 && x >= box.left && x < box.left + box.width &&
                          y >= box.top && y < box.top + box.height;
      start_scroll_ = scroll_;

    }
    if (down && !moved_) {
      const int dx = std::abs(x - start_x_), dy = std::abs(y - start_y_);
      if (dx > 14 || dy > 14) {
        moved_ = true;
        scrolling_ = scroll_candidate_ && dy > dx;
      }
    }
    if (down && scrolling_ && scroll_candidate_) scroll_ = std::clamp(start_scroll_ + start_y_ - y, 0, max_scroll());
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
    const bool consumed = scrolling_;
    if (!down) scrolling_ = false;
    return consumed;
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
    const auto [left, top, width, height] = layout();
    for (int row = 0; row < 17; ++row) {
      const int half = row / 2;
      canvas.fill_rect(kCenter - half, top - 16 + row, half * 2 + 1, 1, border);
    }
    canvas.fill_round_rect(left, top, width, height, 13, border);
    canvas.fill_round_rect(left + 2, top + 2, width - 4, height - 4, 11, fill);
    for (int row = 0; row < 15; ++row) {
      const int half = row / 2;
      canvas.fill_rect(kCenter - half, top - 12 + row, half * 2 + 1, 1, fill);
    }

    const int text_top = top + kPadding;
    const int viewport = std::min(static_cast<int>(lines_.size()), kVisibleRows) * kLineHeight;
    canvas.set_clip_rows(text_top, text_top + viewport);
    int y = text_top - scroll_ % kLineHeight;
    for (size_t row = scroll_ / kLineHeight; row < lines_.size() && y < text_top + viewport;
         ++row, y += kLineHeight) {
      const auto& line = lines_[row];
      const int x = lines_.size() > 2 ? left + kPadding :
          kCenter - hg::Canvas::text_width(line, kScale) / 2;
      canvas.text(x, y, line, kScale, ink);
    }
    canvas.reset_clip();

    const int overflow = max_scroll();
    if (overflow > 0) {
      const int thumb = std::max(16, viewport * viewport / (viewport + overflow));
      const int position = static_cast<int>(static_cast<int64_t>(viewport - thumb) * scroll_ / overflow);
      const int x = left + width - 13;
      canvas.fill_round_rect(x, text_top, 4, viewport, 2, hg::rgb565(73, 49, 34));
      canvas.fill_round_rect(x, text_top + position, 4, thumb, 2, border);
    }
  }

 private:
  static constexpr int kCenter = 233;
  static constexpr int kScale = 2;
  static constexpr int kLineHeight = hg::font::kCellHeight * kScale;
  static constexpr int kVisibleRows = 9;
  static constexpr int kPadding = 14;

  struct Layout { int left, top, width, height; };

  Layout layout() const {
    const int rows = static_cast<int>(lines_.size());
    const int height = 2 * kPadding + std::min(rows, kVisibleRows) * kLineHeight;
    if (rows > 2) return {71, 399 - height, 324, height};
    int width = 132;
    for (const auto& line : lines_)
      width = std::max(width, 2 * kPadding + hg::Canvas::text_width(line, kScale));
    return {kCenter - width / 2, 318, width, height};
  }

  int max_scroll() const {
    return std::max(0, static_cast<int>(lines_.size()) - kVisibleRows) * kLineHeight;
  }

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
    if (message == rendered_text_) return;
    rendered_text_.assign(message);
    lines_ = hg::wrap_text(rendered_text_, 24);
    scroll_ = 0;
    scroll_candidate_ = false;
    start_scroll_ = 0;
  }

  std::string message_;
  std::string rendered_text_;
  std::vector<std::string> lines_;
  uint32_t pressed_at_ = 0;
  uint32_t next_update_ = 0;
  int start_x_ = 0;
  int start_y_ = 0;
  int scroll_ = 0;
  int start_scroll_ = 0;
  bool scroll_candidate_ = false;
  bool scrolling_ = false;
  bool held_ = false;
  bool moved_ = false;
  View view_ = View::Text;
  Startup startup_ = Startup::Done;
  WifiStatus wifi_ = WifiStatus::Offline;
  uint32_t startup_at_ = 0;
};

}  // namespace alter_ego
