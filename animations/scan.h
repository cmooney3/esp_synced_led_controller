#ifndef SCAN_H
#define SCAN_H

#define US_PER_S 1000000
#define SCAN_RATE_HZ 7
#define US_PER_TICK (US_PER_S / SCAN_RATE_HZ)

#include "animation.h"

namespace Scan {

constexpr int kMinSpeed = 5;
constexpr int kMaxSpeed = 15;

class ScanAnimation : public Animation {
  public:
    ScanAnimation(CRGB* leds, int num_leds) : Animation(leds, num_leds) {
    }

    void generateFrame(uint32_t time) {
      uint32_t ticks = time / US_PER_TICK;

      fill_solid(leds_, num_leds_, CRGB(0, 0, 0));

      uint16_t num_positions = (num_leds_ * 2) - 2;
      uint16_t selected_led_ = ticks % num_positions;
      if (selected_led_ >= num_leds_) {
          selected_led_ = num_positions - selected_led_;
      }

      int i = (ticks / (num_positions * 5)) % 3;
      leds_[selected_led_] = colors_[i];
    }

  private:
    CRGB colors_[3] = {CRGB(255, 0, 0), CRGB(0, 255, 0), CRGB(0, 0, 255)};
};

};

#endif // PULSE_H
