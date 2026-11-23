#pragma once

#include <cstdint>

namespace alter_ego {

class Idle {
 public:
  enum class Mode { Animated, Still, Dark };

  explicit Idle(uint32_t now) : activity_(now) {}

  Mode mode(uint32_t now) const {
    if (manual_dark_ || (!finger_down_ && now - activity_ >= 10000)) return Mode::Dark;
    if (!finger_down_ && now - activity_ >= 5000) return Mode::Still;
    return Mode::Animated;
  }

  bool touch(bool down, uint32_t now) {
    if (down && !finger_down_) {
      consume_gesture_ = mode(now) == Mode::Dark;
      manual_dark_ = false;
    }
    finger_down_ = down;
    activity_ = now;
    return !consume_gesture_;
  }

  void wake(uint32_t now) {
    activity_ = now;
    manual_dark_ = false;
  }

  void toggle_display(uint32_t now) {
    manual_dark_ = mode(now) != Mode::Dark;
    activity_ = now;
  }

 private:
  uint32_t activity_;
  bool manual_dark_ = false;
  bool finger_down_ = false;
  bool consume_gesture_ = false;
};

}  // namespace alter_ego
