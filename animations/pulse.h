#ifndef PULSE_H
#define PULSE_H

#include "animation.h"

namespace Pulse {

constexpr int kMinSpeed = 5;
constexpr int kMaxSpeed = 15;

class PulseAnimation : public Animation {
  public:
    PulseAnimation(CRGB* leds, int num_leds, int num_frames) :
         Animation(leds, num_leds, num_frames) {
      fillRandomContrastingColors(c1_, c2_);
      speed_ = random(kMinSpeed, kMaxSpeed);
      angle_ = random(360);
    }

    void generateNextFrame() {
      double mixing_percent = abs(sin(radians(angle_)));
      CRGB mixed = blend(c1_, c2_, static_cast<int>(mixing_percent * 255.0));
      fill_solid(leds_, num_leds_, mixed);
      angle_ += speed_;
    }

  private:
    CRGB c1_, c2_;
    int speed_, angle_;
};

};

#endif // PULSE_H
