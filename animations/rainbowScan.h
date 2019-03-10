#ifndef RAINBOW_SCAN_H
#define RAINBOW_SCAN_H

#define PERIOD_MS 512
#define HUE_DELTA 32

#include "animation.h"

void rainbowScanAnimation(const AnimationInputs& inputs) {
  int offset = TO_MS(inputs.raw_time_us) % PERIOD_MS;
  int starting_h = 255 * offset / PERIOD_MS;
  fill_rainbow(inputs.leds, inputs.num_leds, starting_h, HUE_DELTA);
}

#endif // RAINBOW_SCAN_H
