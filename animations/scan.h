#ifndef SCAN_H
#define SCAN_H

#include "animation.h"

#define SCAN_RATE_HZ 7
#define US_PER_TICK (US_PER_S / SCAN_RATE_HZ)


void scanAnimation(const AnimationInputs& inputs) {
  CRGB colors[3] = {CRGB(255, 0, 0), CRGB(0, 255, 0), CRGB(0, 0, 255)};

  uint32_t ticks = inputs.raw_time_us / US_PER_TICK;

  fill_solid(inputs.leds, inputs.num_leds, CRGB(0, 0, 0));

  uint16_t num_positions = (inputs.num_leds * 2) - 2;
  uint16_t selected_led = ticks % num_positions;
  if (selected_led >= inputs.num_leds) {
      selected_led = num_positions - selected_led;
  }

  int i = (ticks / (num_positions * 5)) % 3;
  inputs.leds[selected_led] = colors[i];
}

#endif // SCAN_H
