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

class SpeechBubble {
 public:
  SpeechBubble() { show_message("Hi, Felix!"); }

  // Replaces the current content and stops the live clock until the next tap.
  void show_message(std::string_view message) {
    showing_clock_ = false;
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
      showing_clock_ = !showing_clock_;
      if (showing_clock_) next_update_ = now;
      else set_text(message_);
    }
    held_ = down;
  }

  template <typename ReadClock>
  void update(uint32_t now, ReadClock read_clock) {
    if (!showing_clock_ || static_cast<int32_t>(now - next_update_) < 0) return;
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
  bool showing_clock_ = false;
};

}  // namespace alter_ego
