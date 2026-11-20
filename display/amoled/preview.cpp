#include <array>
#include <fstream>

#include "portrait.hpp"

namespace {
using stopwatch_art::kSize;
constexpr uint16_t kPixels[] = {
#include "portrait_pixels.inc"
};

class PreviewDisplay final : public hg::Display {
 public:
  hg::DisplayInfo info() const override {
    hg::DisplayInfo result;
    result.width = kSize;
    result.height = kSize;
    result.swap_bytes = true;
    result.round = true;
    return result;
  }

  uint16_t* framebuffer() override { return pixels.data(); }
  void flush(uint16_t first, uint16_t end) override {
    flushed = first == 0 && end == kSize;
  }

  std::array<uint16_t, kSize * kSize> pixels{};
  bool flushed = false;
};
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 1;
  PreviewDisplay panel;
  if (!stopwatch_art::show(panel, kPixels) || !panel.flushed) return 2;

  std::ofstream out(argv[1], std::ios::binary);
  out << "P6\n" << kSize << ' ' << kSize << "\n255\n";
  const auto* wire = reinterpret_cast<const unsigned char*>(panel.pixels.data());
  for (int i = 0; i < kSize * kSize; ++i) {
    const uint16_t pixel = (wire[2 * i] << 8) | wire[2 * i + 1];
    const unsigned char rgb[] = {
        static_cast<unsigned char>(((pixel >> 11) & 31) * 255 / 31),
        static_cast<unsigned char>(((pixel >> 5) & 63) * 255 / 63),
        static_cast<unsigned char>((pixel & 31) * 255 / 31),
    };
    out.write(reinterpret_cast<const char*>(rgb), sizeof(rgb));
  }
  out.close();
  return out ? 0 : 3;
}
