#ifndef RAINBOW_SCAN_H
#define RAINBOW_SCAN_H

#define RAINBOW_SCAN_PERIOD_MS 1000

#include "animation.h"

void rainbowScanAnimation(const AnimationInputs& inputs) {
  int offset = TO_MS(inputs.raw_time_us) % RAINBOW_SCAN_PERIOD_MS;
  int starting_h = 255 * offset / RAINBOW_SCAN_PERIOD_MS;
  fill_rainbow(inputs.leds, inputs.num_leds, starting_h, 0xFF / inputs.num_leds);
}

#endif // RAINBOW_SCAN_H
