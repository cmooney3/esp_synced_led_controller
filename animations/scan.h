#ifndef SCAN_H
#define SCAN_H

#include "animation.h"

#define SCAN_RATE_HZ 7
#define US_PER_TICK (US_PER_S / SCAN_RATE_HZ)


void scanAnimation(uint32_t time, CRGB* leds, int num_leds) {
  CRGB colors[3] = {CRGB(255, 0, 0), CRGB(0, 255, 0), CRGB(0, 0, 255)};

  uint32_t ticks = time / US_PER_TICK;

  fill_solid(leds, num_leds, CRGB(0, 0, 0));

  uint16_t num_positions = (num_leds * 2) - 2;
  uint16_t selected_led = ticks % num_positions;
  if (selected_led >= num_leds) {
      selected_led = num_positions - selected_led;
  }

  int i = (ticks / (num_positions * 5)) % 3;
  leds[selected_led] = colors[i];
}

#endif // SCAN_H
