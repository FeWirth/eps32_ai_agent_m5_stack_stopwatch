#pragma once

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
  bool visible() const { return visible_; }
  void set_state(NetworkState state) { state_ = state; }

  void touch(bool down, int x, int y) {
    if (down) {
      if (!held_) {
        start_x_ = x;
        start_y_ = y;
        moved_ = false;
      }
      last_x_ = x;
      last_y_ = y;
      moved_ |= std::abs(x - start_x_) > 14 || std::abs(y - start_y_) > 14;
    } else if (held_) {
      const int dx = last_x_ - start_x_, dy = last_y_ - start_y_;
      if (std::abs(dx) >= 72 && std::abs(dx) > 2 * std::abs(dy)) {
        visible_ = dx < 0;
      } else if (visible_ && !moved_ && start_x_ >= 82 && start_x_ < 384) {
        if (start_y_ >= 337 && start_y_ < 382) visible_ = false;
        for (int i = 0; i < 2; ++i) {
          const int top = 154 + 75 * i;
          if (start_y_ >= top && start_y_ < top + 62 && state_.saved[i]) selection_ = i;
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
    center(c, 90, "Verbindung", 3, ink);
    center(c, 126, state_.manual < 0 ? "Automatisch" : "Manuelle Auswahl", 2, muted);
    for (int i = 0; i < 2; ++i) {
      const int top = 154 + 75 * i;
      const bool active = state_.active == i;
      const bool selected = state_.manual == i;
      c.fill_round_rect(82, top, 302, 62, 16, selected ? gold : edge);
      c.fill_round_rect(84, top + 2, 298, 58, 14, surface);
      c.text(103, top + 12, i == 0 ? "Heim-WLAN" : "iPhone-Hotspot", 2, state_.saved[i] ? ink : muted);
      const char* label = active ? "Verbunden" : state_.pending == i ? "Verbinde ..." :
                          selected ? "Nicht verbunden" : state_.saved[i] ? "Bereit" : "Nicht eingerichtet";
      c.text(103, top + 34, label, 2, active ? mint : muted);
      c.fill_round_rect(317, top + 18, 47, 25, 12, active ? mint : edge);
      c.fill_round_rect(active ? 341 : 321, top + 22, 17, 17, 8, active ? surface : muted);
    }
    center(c, 314, state_.manual < 0 ? "Netz zum Wechsel antippen" : "Erneut tippen: Automatik", 1, muted);
    center(c, 353, "<  Avatar", 2, gold);
  }

  void draw_indicator(hg::Canvas& c) const {
    constexpr auto gold = hg::rgb565(216, 159, 77);
    constexpr auto muted = hg::rgb565(81, 73, 69);
    c.fill_round_rect(220, 397, 8, 8, 4, visible_ ? muted : gold);
    c.fill_round_rect(238, 397, 8, 8, 4, visible_ ? gold : muted);
  }

 private:
  static void center(hg::Canvas& c, int y, std::string_view text, int scale, uint16_t ink) {
    c.text(233 - hg::Canvas::text_width(text, scale) / 2, y, text, scale, ink);
  }

  NetworkState state_;
  std::optional<int> selection_;
  int start_x_ = 0, start_y_ = 0, last_x_ = 0, last_y_ = 0;
  bool visible_ = false, held_ = false, moved_ = false;
};

}  // namespace alter_ego
