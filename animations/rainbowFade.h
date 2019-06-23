#ifndef RAINBOW_FADE_H
#define RAINBOW_FADE_H

#define RAINBOW_FADE_PERIOD_MS 1000

#include "animation.h"

void rainbowFadeAnimation(const AnimationInputs& inputs) {
  int offset = TO_MS(inputs.raw_time_us) % RAINBOW_FADE_PERIOD_MS;
  int h = 255 * offset / RAINBOW_FADE_PERIOD_MS;
  fill_solid(inputs.leds, inputs.num_leds, CHSV(h, 255, 128));
}

#endif // RAINBOW_FADE_H
