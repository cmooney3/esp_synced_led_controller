#ifndef FLASH_H
#define FLASH_H

#define SCAN_RATE_HZ 7

#include "animation.h"

void flashAnimation(uint32_t time, CRGB* leds, int num_leds) {
  CRGB colors[3] = {CRGB(255, 0, 0), CRGB(0, 255, 0), CRGB(0, 0, 255)};
  fill_solid(leds, num_leds, CHSV(120, 255, 128));
}

#endif // FLASH_H
