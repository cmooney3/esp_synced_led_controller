#ifndef RAINBOW_SCAN_H
#define RAINBOW_SCAN_H

#define PERIOD_MS 1024
#define HUE_DELTA 32

#include "animation.h"

void rainbowScanAnimation(uint32_t time, CRGB* leds, int num_leds) {
  int offset = TO_MS(time) % PERIOD_MS;
  int starting_h = 255 * offset / PERIOD_MS;
  fill_rainbow(leds, num_leds, starting_h, HUE_DELTA);
}

#endif // RAINBOW_SCAN_H
