#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "hg/hal.hpp"
#include "speech_bubble.hpp"
#include "battery_drawer.hpp"

#define HG_ALTER_EGO_SPEECH_BUBBLE 1

namespace alter_ego {
inline constexpr int size = 466;
inline constexpr float pi = 3.14159265f;

struct Point {
  float x, y;
};

class Avatar {
 public:
  SpeechBubble& speech() { return speech_; }
  void set_power(const std::optional<hg::PowerStatus>& power) { battery_.set_power(power); }

  void touch(bool down, int x, int y, uint32_t now) {
    if (battery_.touch(down, x, y, now)) return;
    speech_.touch(down, x, y, now);
    if (down && !held_) awakened_ = now;
    held_ = down;
    if (down) {
      target_yaw_ = std::clamp((x - size / 2) / 650.0f, -.27f, .27f);
      target_pitch_ = std::clamp((y - size / 2) / 870.0f, -.18f, .18f);
    }
  }

  void set_spoons_raised(bool raised, uint32_t now, bool screen_off = false) {
    spoon_from_ = screen_off ? 1.f : spoon_position(now);
    spoon_target_ = raised ? 0.f : 1.f;
    spoon_started_ = now;
  }

  bool spoons_moving(uint32_t now) const {
    return spoon_from_ != spoon_target_ && now - spoon_started_ < kSpoonTravelMs;
  }

  bool draw(hg::Display& display, uint32_t now, bool animate = true) {
    const auto info = display.info();
    if (info.width != size || info.height != size || !info.round ||
        display.framebuffer_rows() || !display.framebuffer()) return false;
    pixels_ = display.framebuffer();
    swapped_ = info.swap_bytes;
    std::fill_n(pixels_, size * size, uint16_t{0});

    if (animate) animation_at_ = now;
    // The animation frequencies are multiples of 0.05 radians per second.
    const float t = static_cast<float>(std::fmod(animation_at_ * .001, 40.0 * pi));
    const auto since = animation_at_ - awakened_;
    const float pulse = since < 1600 ? std::exp(-static_cast<float>(since) / 490.0f) : 0.0f;
    if (animate) {
      yaw_ += ((held_ ? target_yaw_ : .085f * std::sin(t * 1.15f)) - yaw_) * .18f;
      pitch_ += ((held_ ? target_pitch_ : 0.f) - pitch_) * .18f;
      cy_ = std::cos(yaw_);
      sy_ = std::sin(yaw_);
      cp_ = std::cos(pitch_);
      sp_ = std::sin(pitch_);
      bob_ = 5 * std::sin(t * 1.95f) - 8 * pulse;
    }
    sky(t, pulse);

    constexpr Point shoulders[] = {{-139,35},{-116,18},{-90,15},{-53,30},
      {-31,48},{0,58},{31,48},{53,30},{90,15},{116,18},{139,35},
      {115,77},{68,103},{43,124},{-43,124},{-68,103},{-115,77}};
    for (int y = 16; y <= 124; y += 4) {
      for (int x = -140; x <= 140; x += 4) {
        if (!inside(shoulders, x, y)) continue;
        const float gleam = std::max(0.f, 1.f - std::abs(std::abs(x) - 77) / 65.f);
        project(x, y, -26 + 36 * gleam, 139, 83, 43,
                (.40f + gleam * .42f) * (1 - (y - 16) / 166.f), 1);
      }
    }

    constexpr Point head[] = {{-128,-122},{-106,-111},{-76,-94},{-40,-65},{-23,-70},
      {0,-73},{23,-70},{40,-65},{76,-94},{106,-111},{128,-122},{87,-68},{72,-35},
      {71,-20},{82,-4},{62,1},{40,7},{25,21},{13,44},{7,51},{0,53},
      {-7,51},{-13,44},{-25,21},{-40,7},{-62,1},{-82,-4},{-71,-20},{-72,-35},{-87,-68}};
    const float blink = std::clamp(std::abs(static_cast<int>(animation_at_ % 3900) - 3650) / 94.f, .15f, 1.f);
    for (int y = -122; y <= 53; y += 3) {
      for (int x = -128; x <= 128; x += 3) {
        if (!inside(head, x, y)) continue;
        const float ax = std::abs(x);
        const float brow = (ax - 22) / 19.f;
        const float eye = (y - (2 - (ax - 22) * .49f)) / 5.8f;
        const bool socket = brow * brow + eye * eye < 1.35f;
        const float ridge = std::max(0.f, 1.f - ax / 63.f);
        const float face = .58f + .32f * ridge + .11f * std::cos((x + 19) / 34.f);
        const float z = 11 + 36 * ridge + 13 * std::cos((y + 33) / 46.f);
        if (!socket) project(x, y, z, 248, 177, 57, face, 1);
        else if (brow * brow + eye * eye / (blink * blink) < .83f) {
          const bool pupil = std::abs(ax - 22 - yaw_ * 12) < 3.5f && blink > .55f;
          project(x, y, z + 2, 255, 240, 172, pupil ? .25f : .98f, 1);
        }
      }
    }

    const float lowered = spoon_position(now);
    for (const int sign : {-1, 1}) {
      for (int i = 0; i <= 34; ++i) {
        const float u = i / 34.f;
        const float x = 14 + 128 * u;
        const float wave = 34 - 24 * std::sin(u * 2 * pi) + u * 48.f;
        const float width = 13 * std::sin(pi * u) + 2;
        for (int row = -2; row <= 2; ++row) {
          project(sign * x, wave + row * width * .48f - 9 * pulse * u,
                  48 - 32 * u + 7 * std::cos(row * .7f),
                  255, 189, 67, .84f - std::abs(row) * .12f, 1);
        }
      }
      if (lowered < 1.f) spoon(sign, t, pulse, lowered);
    }
    hg::Canvas canvas(pixels_, size, size, swapped_);
    speech_.draw(canvas);
    battery_.draw(canvas, pixels_, now);
    display.flush(0, size);
    return true;
  }

 private:
  template <size_t N>
  static bool inside(const Point (&poly)[N], float x, float y) {
    bool result = false;
    for (size_t i = 0, j = N - 1; i < N; j = i++) {
      const Point a = poly[j], b = poly[i];
      if ((a.y > y) != (b.y > y) && x < a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y)) {
        result = !result;
      }
    }
    return result;
  }

  void dot(int x, int y, int r, int g, int b, float light, int radius) {
    if (x < 2 || x >= size - 2 || y < 2 || y >= size - 2) return;
    const uint16_t word = ((static_cast<int>(r * light) >> 3) << 11) |
                          ((static_cast<int>(g * light) >> 2) << 5) |
                          (static_cast<int>(b * light) >> 3);
    const uint16_t color = swapped_ ? static_cast<uint16_t>((word << 8) | (word >> 8)) : word;
    auto* pixel = pixels_ + y * size + x;
    *pixel = color;
    if (radius) pixel[-1] = pixel[1] = pixel[-size] = pixel[size] = color;
  }

  void project(float x, float y, float z, int r, int g, int b, float light, int radius, bool clip_round = false) {
    const float rx = x * cy_ + z * sy_;
    const float rz = z * cy_ - x * sy_;
    const float ry = y * cp_ - rz * sp_;
    const float depth = y * sp_ + rz * cp_;
    const float scale = 430.f / (430.f - depth);
    const int sx = static_cast<int>(233 + rx * scale);
    const int sy = static_cast<int>(224 + bob_ + ry * scale);
    if (clip_round && (sx - 233) * (sx - 233) + (sy - 233) * (sy - 233) > 231 * 231) return;
    dot(sx, sy, r, g, b, light, radius);
  }

  void sky(float t, float pulse) {
    for (int i = 0; i < 360; ++i) {
      const float angle = i * pi / 180;
      const float x = std::cos(angle), y = std::sin(angle);
      dot(233 + int(210 * x), 233 + int(210 * y), 213, 150, 71, i % 30 == 0 ? .98f : .63f, 0);
      dot(233 + int(149 * x), 208 + int(149 * y), 81, 115, 189, .62f + .30f * pulse, 0);
      dot(233 + int((116 + 48 * pulse) * x), 373 + int((13 + 14 * pulse) * y),
          239, 154, 68, .25f + .27f * (1 + y) + .12f * pulse, 0);
    }
    // Engraved full moon above the character.
    for (int y = -16; y <= 16; y += 2) {
      for (int x = -16; x <= 16; x += 2) {
        if (x * x + y * y > 256) continue;
        const float light = .65f + .13f * std::cos(x * .8f + y * .37f);
        dot(233 + x, 62 + y, 248, 181, 112, light, 0);
      }
    }
    uint32_t seed = 1097;
    for (int i = 0; i < 112; ++i) {
      seed = seed * 1664525u + 1013904223u;
      const int x = int((seed >> 16) % 399) - 199;
      seed = seed * 1664525u + 1013904223u;
      const int y = int((seed >> 16) % 399) - 199;
      if (x * x + y * y > 202 * 202) continue;
      const float glow = .47f + .22f * std::sin(t * 1.6f + i);
      dot(233 + x, 233 + y, i % 3 == 0 ? 218 : 94, 133, 164, glow, i % 12 == 0);
    }
  }

  static constexpr uint32_t kSpoonTravelMs = 760;

  float spoon_position(uint32_t now) const {
    if (!spoons_moving(now)) return spoon_target_;
    const float u = (now - spoon_started_) / static_cast<float>(kSpoonTravelMs);
    const float eased = u * u * (3 - 2 * u);
    return spoon_from_ + (spoon_target_ - spoon_from_) * eased;
  }

  void spoon(int sign, float t, float pulse, float lowered) {
    const float phase = t * 2.1f + sign;
    const float ox = sign * (158 + 8 * std::cos(phase) + 12 * pulse + 22 * lowered);
    const float oy = -31 + 13 * std::sin(phase) - 17 * pulse + 290 * lowered;
    const float z = 17 * std::cos(phase);
    const float tilt = (-.48f + .38f * lowered) * sign + .15f * std::sin(phase);
    const float c = std::cos(tilt), s = std::sin(tilt);
    for (int y = -24; y <= 43; y += 3) {
      for (int x = -15; x <= 15; x += 3) {
        const float bowl = (x * x) / 225.f + ((y + 7) * (y + 7)) / 361.f;
        if (bowl > 1.f && !(y >= 8 && std::abs(x) <= (y > 32 ? 5 : 3))) continue;
        const float light = std::clamp(.51f + .32f * bowl - x / 38.f + pulse * .11f, .25f, 1.f);
        project(ox + x * c + y * s, oy + y * c - x * s, z + 4 * std::max(0.f, 1 - bowl),
                248, 232, 206, light * (1 - .28f * lowered), 1, true);
      }
    }
  }

  SpeechBubble speech_;
  BatteryDrawer battery_;
  uint16_t* pixels_ = nullptr;
  uint32_t spoon_started_ = 0;
  float spoon_from_ = 0, spoon_target_ = 0;
  uint32_t animation_at_ = 0;
  uint32_t awakened_ = uint32_t{0} - 1600u;
  bool held_ = false, swapped_ = false;
  float yaw_ = 0, pitch_ = 0, target_yaw_ = 0, target_pitch_ = 0;
  float cy_ = 1, sy_ = 0, cp_ = 1, sp_ = 0, bob_ = 0;
};
}  // namespace alter_ego
