#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

#include "hg/canvas.hpp"

namespace alter_ego {

class PomodoroPage {
 public:
  void start(int minutes, uint32_t now, std::string_view project, std::string_view task) {
    break_minutes_ = 0;
    completion_pending_ = false;
    break_request_ = break_completed_ = 0;
    minutes_ = minutes;
    stored_ms_ = duration_ms();
    started_at_ = now;
    project_ = project;
    task_ = task;
    running_ = true;
  }

  void draw(hg::Canvas& c, uint32_t now, std::string_view project = "Kein Projekt",
            std::string_view task = "Keine Aufgabe") const {
    const auto ms = remaining(now);
    const unsigned seconds = (ms + 999) / 1000;
    const bool active = running_ && ms != 0;
    const bool finished = ms == 0;
    const bool paused = !active && !finished && ms < duration_ms();
    center(c, 43, break_minutes_ ? "PAUSE" : "POMODORO", 2, muted_);

    const bool ready = !running_ && ms == duration_ms();
    draw_progress(c, ready ? duration_ms() : duration_ms() - ms, duration_ms());

    char time[12];
    std::snprintf(time, sizeof(time), "%02u:%02u", seconds / 60, seconds % 60);
    center(c, 128, time, 7, ink_);
    center(c, 192, finished ? "Fokus fertig" : break_minutes_ ? "Regeneration" :
           active ? "Fokus" : paused ? "Pausiert" : "Bereit", 2, muted_);

    if (break_minutes_) {
      const auto lines = hg::wrap_text(tip_, 30);
      int y = 230;
      for (size_t i = 0; i < std::min<size_t>(4, lines.size()); ++i, y += 20)
        center(c, y, lines[i], 2, ink_);
      button(c, 138, 367, 190, 44, "Pause beenden", false);
      return;
    }

    center(c, 258, project_.empty() ? project : std::string_view(project_).substr(0, 32), 2, muted_);
    center(c, 279, project_.empty() ? task : std::string_view(task_).substr(0, 21), 3, ink_);

    if (finished) {
      button(c, 68, 315, 160, 48, "5 min Pause", false, true);
      button(c, 238, 315, 160, 48, "10 min Pause", false, true);
      button(c, 138, 367, 190, 44, "Ohne Pause", false);
      return;
    }
    button(c, 118, 318, 110, 40, "25 min", minutes_ == 25);
    button(c, 238, 318, 110, 40, "50 min", minutes_ == 50);
    button(c, 138, 367, 190, 44,
           active ? "Pause" : paused ? "Fortsetzen" : finished ? "Neu starten" : "Start",
           false, true);
  }

  void tap(int x, int y, uint32_t now) {
    const bool bottom = x >= 138 && x < 328 && y >= 367 && y < 411;
    if (break_minutes_) {
      if (bottom) reset_focus();
      return;
    }
    if (remaining(now) == 0) {
      if (bottom) reset_focus();
      if (y >= 315 && y < 363) {
        if (x >= 68 && x < 228) start_break(5, now);
        if (x >= 238 && x < 398) start_break(10, now);
      }
      return;
    }
    if (y >= 318 && y < 358) {
      if (x >= 118 && x < 228) select_duration(25);
      if (x >= 238 && x < 348) select_duration(50);
    }
    if (!bottom) return;
    const auto ms = remaining(now);
    if (running_ && ms != 0) {
      stored_ms_ = ms;
      running_ = false;
    } else {
      started_at_ = now;
      running_ = true;
    }
  }

  bool update(uint32_t now) {
    if (!running_ || remaining(now) != 0) return false;
    if (break_minutes_) {
      const auto minutes = break_minutes_;
      reset_focus();
      break_completed_ = minutes;
    } else {
      stored_ms_ = 0;
      running_ = false;
      completion_pending_ = true;
    }
    return true;
  }

  bool completion_pending() const { return completion_pending_; }
  void clear_completion() { completion_pending_ = false; }
  int take_break_request() { return std::exchange(break_request_, 0); }
  int take_break_completion() { return std::exchange(break_completed_, 0); }
  bool set_break_tip(std::string_view tip) {
    if (!break_minutes_) return false;
    tip_ = tip.substr(0, 120);
    return true;
  }

  uint32_t refresh_delay(uint32_t now) const {
    const auto ms = remaining(now);
    if (!running_ || ms == 0) return 10000;  // Input and wake request an immediate redraw.
    return (ms - 1) % 1000 + 1;
  }

 private:
  static constexpr auto accent_ = hg::rgb565(240, 173, 97);
  static constexpr auto ink_ = hg::rgb565(250, 236, 215);
  static constexpr auto muted_ = hg::rgb565(169, 157, 146);
  static constexpr auto edge_ = hg::rgb565(64, 53, 48);
  static constexpr auto surface_ = hg::rgb565(27, 22, 21);

  // Clockwise angle in 1/4096 turns, beginning at twelve o'clock.
  static int angle_at(int x, int y) {
    static const auto octant = [] {
      std::array<uint16_t, 257> angles{};
      for (size_t i = 0; i < angles.size(); ++i)
        angles[i] = std::lround(std::atan(i / 256.f) * (4096.f / 6.2831853f));
      return angles;
    }();
    const int ax = std::abs(x), ay = std::abs(y);
    const int quarter = ax <= ay ? octant[256 * ax / ay] : 1024 - octant[256 * ay / ax];
    const int half = y <= 0 ? quarter : 2048 - quarter;
    return x >= 0 ? half : 4096 - half;
  }

  static void draw_progress(hg::Canvas& c, uint32_t elapsed, uint32_t duration) {
    constexpr int center = 233;
    constexpr int inner = 214, outer = 219;
    constexpr int gap = 34, sweep = 4096 - 2 * gap;
    const int visible = static_cast<uint64_t>(elapsed) * sweep / duration;
    for (int y = -outer; y <= outer; ++y) {
      const int last = std::sqrt(outer * outer - y * y);
      const int first = std::abs(y) < inner ? std::sqrt(inner * inner - y * y) : 0;
      for (int offset = first; offset <= last; ++offset) {
        const int rr = offset * offset + y * y;
        // Fade over one pixel at either radial edge without shifting the circle.
        const int coverage = std::clamp(std::min(rr - inner * inner, outer * outer - rr) * 256 /
                                        (2 * inner), 0, 256);
        if (coverage == 0) continue;
        for (const int x : {-offset, offset}) {
          const int position = angle_at(x, y) - gap;
          const int cap = std::min(position, sweep - position);
          if (cap < 0) continue;
          const int alpha = std::min(coverage, cap * 64);
          const int lit = std::clamp((visible - position) * 64, 0, 256);
          const auto channel = [&](int start, int end, int track) {
            const int gradient = start + (end - start) * position / sweep;
            return (track + (gradient - track) * lit / 256) * alpha / 256;
          };
          c.pixel(center + x, center + y, hg::rgb565(channel(255, 239, 48),
                                                   channel(199, 91, 29),
                                                   channel(103, 36, 22)));
        }
      }
    }
  }

  static void center(hg::Canvas& c, int y, std::string_view text, int scale, uint16_t color) {
    c.text(233 - hg::Canvas::text_width(text, scale) / 2, y, text, scale, color);
  }

  static void button(hg::Canvas& c, int x, int y, int width, int height,
                     std::string_view label, bool selected, bool primary = false) {
    c.fill_round_rect(x, y, width, height, 15, primary || selected ? accent_ : edge_);
    if (!primary) c.fill_round_rect(x + 1, y + 1, width - 2, height - 2, 14, surface_);
    c.text(x + (width - hg::Canvas::text_width(label, 2)) / 2, y + (height - 14) / 2, label, 2,
           primary ? surface_ : selected ? accent_ : muted_);
  }

  uint32_t duration_ms() const { return (break_minutes_ ? break_minutes_ : minutes_) * 60000u; }

  void start_break(int minutes, uint32_t now) {
    break_minutes_ = break_request_ = minutes;
    completion_pending_ = false;
    started_at_ = now;
    stored_ms_ = duration_ms();
    running_ = true;
    tip_ = "Hermes erstellt einen Tipp ...";
  }

  void reset_focus() {
    break_minutes_ = break_request_ = break_completed_ = 0;
    completion_pending_ = false;
    running_ = false;
    stored_ms_ = duration_ms();
    project_.clear();
    task_.clear();
  }

  uint32_t remaining(uint32_t now) const {
    return running_ ? stored_ms_ - std::min(stored_ms_, now - started_at_) : stored_ms_;
  }

  void select_duration(int minutes) {
    if (minutes == minutes_) return;
    minutes_ = minutes;
    reset_focus();
  }

  int minutes_ = 25;
  uint32_t stored_ms_ = 25 * 60000u;
  uint32_t started_at_ = 0;
  bool running_ = false;
  bool completion_pending_ = false;
  int break_minutes_ = 0;
  int break_request_ = 0;
  int break_completed_ = 0;
  std::string tip_;
  std::string project_, task_;
};

}  // namespace alter_ego
