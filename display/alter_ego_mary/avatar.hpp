#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "hg/hal.hpp"

namespace alter_ego {
inline constexpr int size = 466;
inline constexpr float pi = 3.14159265f;

struct Point {
  float x, y;
};

class Avatar {
 public:
  void touch(bool down, int x, int y, uint32_t now) {
    if (down && !held_) awakened_ = now;
    held_ = down;
    if (down) {
      target_yaw_ = std::clamp((x - size / 2) / 650.0f, -.27f, .27f);
      target_pitch_ = std::clamp((y - size / 2) / 870.0f, -.18f, .18f);
    }
  }

  bool draw(hg::Display& display, uint32_t now) {
    const auto info = display.info();
    if (info.width != size || info.height != size || !info.round ||
        display.framebuffer_rows() || !display.framebuffer()) return false;
    pixels_ = display.framebuffer();
    swapped_ = info.swap_bytes;
    std::fill_n(pixels_, size * size, uint16_t{0});

    // The animation frequencies are multiples of 0.05 radians per second.
    const float t = static_cast<float>(std::fmod(now * .001, 40.0 * pi));
    const auto since = now - awakened_;
    const float pulse = since < 1600 ? std::exp(-static_cast<float>(since) / 490.0f) : 0.0f;
    yaw_ += ((held_ ? target_yaw_ : .085f * std::sin(t * 1.15f)) - yaw_) * .18f;
    pitch_ += ((held_ ? target_pitch_ : 0.f) - pitch_) * .18f;
    cy_ = std::cos(yaw_);
    sy_ = std::sin(yaw_);
    cp_ = std::cos(pitch_);
    sp_ = std::sin(pitch_);
    bob_ = 5 * std::sin(t * 1.95f) - 8 * pulse;
    sky(t, pulse);

    piepi(now, t, pulse);
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

  void project(float x, float y, float z, int r, int g, int b, float light, int radius) {
    const float rx = x * cy_ + z * sy_;
    const float rz = z * cy_ - x * sy_;
    const float ry = y * cp_ - rz * sp_;
    const float depth = y * sp_ + rz * cp_;
    const float scale = 430.f / (430.f - depth);
    dot(static_cast<int>(233 + rx * scale), static_cast<int>(224 + bob_ + ry * scale),
        r, g, b, light, radius);
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

  static float ellipse(float x, float y, float rx, float ry) {
    return (x / rx) * (x / rx) + (y / ry) * (y / ry);
  }

  static float faceDepth(float x, float y) {
    return 6 + 44 * std::sqrt(std::max(0.f, 1 - ellipse(x, y, 89, 112)));
  }

  void piepi(uint32_t now, float t, float pulse) {
    constexpr Point ear[] = {{43,-78},{66,-97},{103,-105},{97,-67},{82,-40},{58,-45}};
    constexpr Point arm[] = {{52,17},{67,-12},{92,-18},{115,-32},{127,-46},
      {132,-39},{138,-35},{137,-21},{128,-2},{107,16},{76,34},{59,33}};
    const float blink = std::clamp(std::abs(static_cast<int>(now % 3900) - 3650) / 90.f, .13f, 1.f);
    for (int sign : {-1, 1}) {
      for (int y = -105; y < -38; y += 3) {
        for (int x = 43; x <= 103; x += 3) {
          if (!inside(ear, x, y)) continue;
          const bool tip = x > 80 + (y + 102) * .29f;
          project(sign * x, y, -6 + (103 - x) * .25f,
                  tip ? 118 : 255, tip ? 78 : 159, tip ? 124 : 172,
                  .62f + (103 - x) * .006f, 1);
        }
      }
      for (int y = 72; y <= 117; y += 3) {
        for (int x = 12; x <= 65; x += 3) {
          const float rim = ellipse(x - 37, y - 96, 23, 21);
          if (rim <= 1) project(sign * x, y, 4, 251, 150, 162, .55f + .29f * (1 - rim), 1);
        }
      }
    }

    for (int y = -111; y <= 111; y += 3) {
      for (int x = -90; x <= 90; x += 3) {
        const float ax = std::abs(x);
        const float rim = ellipse(x, y, 89, 112);
        if (rim > 1) continue;
        const bool eye = ellipse(ax - 27, y + 24, 7.7f, 14 * blink) <= 1;
        const bool glint = eye && blink > .55f && ellipse(ax - 25, y + 29, 3.1f, 4.6f) <= 1;
        const bool mouth = ellipse(x, y + 4, 6.5f, 9.5f) <= 1;
        const bool cheek = ellipse(ax - 47, y + 5, 10, 4.8f) <= 1;
        const float hatch = .055f * std::cos(x * .43f + y * .23f);
        const float light = std::clamp(.67f + .16f * (1 - rim) - y * .0013f + hatch, .44f, .99f);
        if (glint) project(x, y, faceDepth(x, y), 255, 244, 213, 1, 1);
        else if (eye || mouth) project(x, y, faceDepth(x, y), 107, 53, 77, .67f, 1);
        else if (cheek) project(x, y, faceDepth(x, y), 245, 109, 146, light, 1);
        else if (rim > .90f) project(x, y, faceDepth(x, y), 255, 207, 154, .89f + hatch, 1);
        else project(x, y, faceDepth(x, y), 255, 169, 185, light, 1);
      }
    }

    for (int sign : {-1, 1}) {
      const float sway = 5 * std::sin(t * 1.95f + sign * .7f) + 11 * pulse;
      for (int y = -46; y <= 34; y += 3) {
        for (int x = 52; x <= 138; x += 3) {
          if (!inside(arm, x, y)) continue;
          const float reach = (x - 52) / 86.f;
          const float light = .74f - y * .003f + .07f * std::cos(x * .26f - y * .28f);
          project(sign * x, y - reach * sway, 44 - reach * 27,
                  255, 172, 180, light, 1);
        }
      }
    }

    for (int i = 0; i <= 72; ++i) {
      const float u = i / 72.f;
      const float angle = .9f + u * 2 * pi * 1.14f;
      const float radius = 24 * (1 - u);
      const float x = radius * std::cos(angle);
      const float y = -77 + radius * std::sin(angle);
      const float depth = faceDepth(x, y);
      project(x + 1.7f, y + 2, depth + 2, 165, 73, 103, .74f, 1);
      project(x, y, depth + 4, 255, 211, 162, .96f, 1);
    }
  }

  uint16_t* pixels_ = nullptr;
  uint32_t awakened_ = uint32_t{0} - 1600u;
  bool held_ = false, swapped_ = false;
  float yaw_ = 0, pitch_ = 0, target_yaw_ = 0, target_pitch_ = 0;
  float cy_ = 1, sy_ = 0, cp_ = 1, sp_ = 0, bob_ = 0;
};
}  // namespace alter_ego
