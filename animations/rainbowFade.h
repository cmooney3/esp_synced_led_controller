#ifndef RAINBOW_FADE_H
#define RAINBOW_FADE_H

#define PERIOD_MS 4200

#include "animation.h"

void rainbowFadeAnimation(uint32_t time, CRGB* leds, int num_leds) {
  int offset = TO_MS(time) % PERIOD_MS;
  int h = 255 * offset / PERIOD_MS;
  fill_solid(leds, num_leds, CHSV(h, 255, 128));
}

#endif // RAINBOW_FADE_H
