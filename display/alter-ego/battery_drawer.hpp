#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>

#include "hg/battery.hpp"
#include "hg/canvas.hpp"
#include "hg/hal.hpp"

namespace alter_ego {

class BatteryDrawer {
 public:
  void set_power(const std::optional<hg::PowerStatus>& power) {
    percent_.reset();
    estimated_ = false;
    on_usb_ = power && power->external_power == true;
    if (!power || power->battery_present == false) return;
    if (power->battery_percent) {
      percent_ = std::min<uint8_t>(*power->battery_percent, 100);
    } else if (power->battery_mv && *power->battery_mv >= 2000 && *power->battery_mv <= 4500) {
      percent_ = hg::estimate_liion_curve_percent(*power->battery_mv);
      estimated_ = true;
    }
  }

  // Consume gestures that begin at the top handle or while the drawer is visible.
  bool touch(bool down, int x, int y, uint32_t now) {
    if (down && !finger_down_) {
      dragging_ = open_ || progress(now) > 0 || (y <= 62 && std::abs(x - 233) <= 95);
      start_x_ = x;
      start_y_ = y;
      start_progress_ = progress(now);
      drag_progress_ = start_progress_;
    }
    finger_down_ = down;
    if (!dragging_) return false;
    if (down) {
      const int dy = y - start_y_;
      if (std::abs(dy) >= std::abs(x - start_x_) || drag_progress_ != start_progress_) {
        drag_progress_ = std::clamp(start_progress_ + dy / 165.f, 0.f, 1.f);
      }
    } else {
      open_ = open_ ? drag_progress_ > .72f : drag_progress_ >= .32f;
      animation_from_ = drag_progress_;
      animation_at_ = now;
      dragging_ = false;
    }
    return true;
  }

  void draw(hg::Canvas& canvas, uint16_t* pixels, uint32_t now) const {
    constexpr uint16_t gold = hg::rgb565(201, 145, 69);
    constexpr uint16_t ink = hg::rgb565(255, 232, 191);
    constexpr uint16_t muted = hg::rgb565(174, 155, 134);
    constexpr uint16_t surface = hg::rgb565(17, 20, 33);
    const float visible = dragging_ ? drag_progress_ : progress(now);
    if (visible <= 0) {
      canvas.fill_round_rect(213, 34, 40, 4, 2, gold);
      return;
    }

    const int top = -190 + static_cast<int>(252 * visible);
    canvas.fill_round_rect(103, top, 260, 190, 24, gold);
    canvas.fill_round_rect(105, top + 2, 256, 186, 22, surface);
    center_text(canvas, top + 24, "Akku", 2, muted);
    char value[8] = "--";
    if (percent_) {
      std::snprintf(value, sizeof(value), "%s%u%%", estimated_ ? "~" : "",
                    static_cast<unsigned>(*percent_));
    }
    center_text(canvas, top + 55, value, 5, ink);
    canvas.fill_round_rect(143, top + 104, 180, 12, 6, hg::rgb565(47, 48, 61));
    if (percent_ && *percent_ > 0) {
      const int width = 180 * *percent_ / 100;
      canvas.fill_round_rect(143, top + 104, width, 12, std::min(6, width / 2), gold);
    }
    if (on_usb_) center_text(canvas, top + 132, "USB verbunden", 2, muted);
    canvas.fill_round_rect(214, top + 169, 38, 4, 2, gold);

    // The same round crop is visible on the Mac and behind the AMOLED glass.
    constexpr int center = 233;
    constexpr int radius = 232;
    for (int y = std::max(0, top); y < top + 190; ++y) {
      const int dy = y - center;
      const int squared = radius * radius - dy * dy;
      auto* row = pixels + y * canvas.width();
      if (squared <= 0) {
        std::fill(row + 103, row + 363, uint16_t{0});
      } else {
        const int half = static_cast<int>(std::sqrt(static_cast<float>(squared)));
        std::fill(row + 103, row + std::clamp(center - half, 103, 363), uint16_t{0});
        std::fill(row + std::clamp(center + half + 1, 103, 363), row + 363, uint16_t{0});
      }
    }
  }

 private:
  float progress(uint32_t now) const {
    const float fraction = std::min((now - animation_at_) / 210.f, 1.f);
    const float remaining = 1.f - fraction;
    const float target = open_ ? 1.f : 0.f;
    return target + (animation_from_ - target) * remaining * remaining * remaining;
  }

  static void center_text(hg::Canvas& canvas, int y, std::string_view text, int scale, uint16_t ink) {
    canvas.text(233 - hg::Canvas::text_width(text, scale) / 2, y, text, scale, ink);
  }

  std::optional<uint8_t> percent_;
  bool estimated_ = false;
  bool on_usb_ = false;
  bool open_ = false;
  bool finger_down_ = false;
  bool dragging_ = false;
  int start_x_ = 0;
  int start_y_ = 0;
  float start_progress_ = 0;
  float drag_progress_ = 0;
  float animation_from_ = 0;
  uint32_t animation_at_ = 0;
};

}  // namespace alter_ego
