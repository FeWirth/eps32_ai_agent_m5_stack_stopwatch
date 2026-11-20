#pragma once

#include <cstdint>

#include "hg/hal.hpp"

namespace stopwatch_art {

inline constexpr int kSize = 466;

// Source pixels are numeric RGB565, independent of the panel's byte order.
inline bool show(hg::Display& display, const uint16_t (&pixels)[kSize * kSize]) {
  const auto info = display.info();
  if (!info.round || info.width != kSize || info.height != kSize ||
      display.framebuffer_rows() != 0) {
    return false;
  }
  auto* framebuffer = display.framebuffer();
  if (!framebuffer) return false;

  for (int i = 0; i < kSize * kSize; ++i) {
    const uint16_t pixel = pixels[i];
    framebuffer[i] = info.swap_bytes ? static_cast<uint16_t>((pixel >> 8) | (pixel << 8)) : pixel;
  }
  display.flush(0, kSize);
  return true;
}

}  // namespace stopwatch_art
