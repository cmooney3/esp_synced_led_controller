#ifndef RAINBOW_H
#define RAINBOW_H

#include "animation.h"

namespace Rainbow {

constexpr int kMinHueStep = 4;
constexpr int kMaxHueStep = 15;

constexpr int kMinSpeed = 3;
constexpr int kMaxSpeed = 12;


class RainbowAnimation : public Animation {
  public:
    RainbowAnimation(CRGB* leds, int num_leds, int num_frames) :
           Animation(leds, num_leds, num_frames) {
      offset_ = random(255);
      speed_ = random(kMinSpeed, kMaxSpeed) * randomDirection();
      // This value sets how much the hue steps ahead for each pixel, effectively
      // controlling how wide the rainbow pattern is.
      hue_step_ = random(kMinHueStep, kMaxHueStep);
    }

    void generateNextFrame() {
      fill_rainbow(leds_, num_leds_, offset_, hue_step_);
      offset_ = (offset_ + speed_) % 255;
    }
  
  private:
    int speed_, hue_step_, offset_;
};

};

#endif // RAINBOW_H
