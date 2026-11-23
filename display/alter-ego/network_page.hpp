#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string_view>

#include "hg/canvas.hpp"

namespace alter_ego {

struct NetworkState {
  int active = -1;
  int manual = -1;
  int pending = -1;
  bool saved[2] = {};
};

class NetworkPage {
 public:
  struct VolumeChange { int percent; bool save; };

  void set_volume(int percent) {
    if (!dragging_volume_) volume_ = percent;
  }

  std::optional<VolumeChange> take_volume_change() {
    const auto change = volume_change_;
    volume_change_.reset();
    return change;
  }
  bool visible() const { return visible_; }
  void set_state(NetworkState state) { state_ = state; }

  void touch(bool down, int x, int y) {
    if (down) {
      if (!held_) {
        start_x_ = x;
        start_y_ = y;
        moved_ = false;
        dragging_volume_ = visible_ && x >= 86 && x < 380 && y >= 294 && y < 347;
      }
      last_x_ = x;
      last_y_ = y;
      if (dragging_volume_) {
        volume_ = (std::clamp(x, kTrackStart, kTrackEnd) - kTrackStart) * 100 /
                  (kTrackEnd - kTrackStart);
        volume_change_ = VolumeChange{volume_, false};
      }
      moved_ |= std::abs(x - start_x_) > 14 || std::abs(y - start_y_) > 14;
    } else if (dragging_volume_) {
      dragging_volume_ = false;
      volume_change_ = VolumeChange{volume_, true};
    } else if (held_) {
      const int dx = last_x_ - start_x_, dy = last_y_ - start_y_;
      if (std::abs(dx) >= 72 && std::abs(dx) > 2 * std::abs(dy)) {
        visible_ = dx < 0;
      } else if (visible_ && !moved_ && start_x_ >= 82 && start_x_ < 384) {
        if (start_y_ >= 352 && start_y_ < 388) visible_ = false;
        for (int i = 0; i < 2; ++i) {
          const int top = kNetworkTop + kNetworkStep * i;
          if (start_y_ >= top && start_y_ < top + kNetworkHeight && state_.saved[i]) selection_ = i;
        }
      }
    }
    held_ = down;
  }

  std::optional<int> take_selection() {
    auto result = selection_;
    selection_.reset();
    return result;
  }

  void draw(hg::Canvas& c) const {
    constexpr auto gold = hg::rgb565(216, 159, 77);
    constexpr auto muted = hg::rgb565(173, 160, 144);
    constexpr auto ink = hg::rgb565(255, 235, 201);
    constexpr auto mint = hg::rgb565(125, 230, 179);
    constexpr auto edge = hg::rgb565(64, 55, 52);
    constexpr auto surface = hg::rgb565(23, 23, 34);
    center(c, 65, "Einstellungen", 3, ink);
    center(c, 99, state_.manual < 0 ? "Automatisch" : "Manuelle Auswahl", 2, muted);
    for (int i = 0; i < 2; ++i) {
      const int top = kNetworkTop + kNetworkStep * i;
      const bool active = state_.active == i;
      const bool selected = state_.manual == i;
      c.fill_round_rect(82, top, 302, kNetworkHeight, 16, selected ? gold : edge);
      c.fill_round_rect(84, top + 2, 298, kNetworkHeight - 4, 14, surface);
      c.text(103, top + 12, i == 0 ? "Heim-WLAN" : "iPhone-Hotspot", 2, state_.saved[i] ? ink : muted);
      const char* label = active ? "Verbunden" : state_.pending == i ? "Verbinde ..." :
                          selected ? "Nicht verbunden" : state_.saved[i] ? "Bereit" : "Nicht eingerichtet";
      c.text(103, top + 34, label, 2, active ? mint : muted);
      c.fill_round_rect(317, top + 18, 47, 25, 12, active ? mint : edge);
      c.fill_round_rect(active ? 341 : 321, top + 22, 17, 17, 8, active ? surface : muted);
    }
    c.fill_round_rect(88, 260, 290, 83, 16, edge);
    c.fill_round_rect(90, 262, 286, 79, 14, surface);
    c.text(105, 273, "Lautstärke", 2, ink);
    char value[6];
    std::snprintf(value, sizeof(value), "%u%%", static_cast<unsigned>(volume_));
    c.text(360 - hg::Canvas::text_width(value, 2), 273, value, 2, gold);

    const int knob = kTrackStart + (kTrackEnd - kTrackStart) * volume_ / 100;
    c.fill_round_rect(kTrackStart, 316, kTrackEnd - kTrackStart + 1, 6, 3, edge);
    if (knob > kTrackStart)
      c.fill_rect(kTrackStart, 316, knob - kTrackStart, 6, gold);
    c.fill_round_rect(knob - 9, 310, 19, 18, 8, ink);
    center(c, 364, "<  Avatar", 2, gold);
  }

  void draw_indicator(hg::Canvas& c) const {
    constexpr auto gold = hg::rgb565(216, 159, 77);
    constexpr auto muted = hg::rgb565(81, 73, 69);
    c.fill_round_rect(220, 397, 8, 8, 4, visible_ ? muted : gold);
    c.fill_round_rect(238, 397, 8, 8, 4, visible_ ? gold : muted);
  }

 private:
  static constexpr int kNetworkTop = 125;
  static constexpr int kNetworkStep = 67;
  static constexpr int kNetworkHeight = 58;
  static constexpr int kTrackStart = 108;
  static constexpr int kTrackEnd = 358;

  static void center(hg::Canvas& c, int y, std::string_view text, int scale, uint16_t ink) {
    c.text(233 - hg::Canvas::text_width(text, scale) / 2, y, text, scale, ink);
  }

  NetworkState state_;
  std::optional<VolumeChange> volume_change_;
  int volume_ = 70;
  bool dragging_volume_ = false;
  std::optional<int> selection_;
  int start_x_ = 0, start_y_ = 0, last_x_ = 0, last_y_ = 0;
  bool visible_ = false, held_ = false, moved_ = false;
};

}  // namespace alter_ego
