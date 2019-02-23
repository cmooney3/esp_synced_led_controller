#ifndef SCAN_H
#define SCAN_H

#define US_PER_S 1000000

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
      uint32_t seconds = time / US_PER_S;
      fill_solid(leds_, num_leds_, (seconds % 2 == 0) ? c1_ : c2_);
    }

  private:
    CRGB c1_, c2_;
};

};

#endif // PULSE_H
