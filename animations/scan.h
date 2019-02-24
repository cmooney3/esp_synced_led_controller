#ifndef SCAN_H
#define SCAN_H

#define US_PER_S 1000000
#define SCAN_RATE_HZ 10
#define US_PER_TICK (US_PER_S / SCAN_RATE_HZ)

#include "animation.h"

namespace Scan {

constexpr int kMinSpeed = 5;
constexpr int kMaxSpeed = 15;

class ScanAnimation : public Animation {
  public:
    ScanAnimation(CRGB* leds, int num_leds) : Animation(leds, num_leds) {
      fillRandomContrastingColors(c1_, c2_);
    }

    void generateFrame(uint32_t time) {
      uint32_t ticks = time / US_PER_TICK;

      fill_solid(leds_, num_leds_, c1_);

      uint16_t num_positions = (num_leds_ * 2) - 1;
      uint16_t selected_led_ = ticks % num_positions;
      if (selected_led_ >= num_leds_) {
          selected_led_ = num_positions - selected_led_;
      }

      leds_[selected_led_] = c2_;
    }

  private:
    CRGB c1_, c2_;
};

};

#endif // PULSE_H
