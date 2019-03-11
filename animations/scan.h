#ifndef SCAN_H
#define SCAN_H

#include "animation.h"

#define SCAN_RATE_HZ 7
#define US_PER_TICK (US_PER_S / SCAN_RATE_HZ)

#define FRAME_DURATION_MS 60


void scanAnimation(const AnimationInputs& inputs) {
  int frame = frame_number(inputs, FRAME_DURATION_MS);

  fill_solid(inputs.leds, inputs.num_leds, CRGB(0, 0, 0));

  int selected_led = cylon_signal(frame, 1, inputs.num_leds);

  inputs.leds[selected_led] = random_color(inputs, 0);
}

#endif // SCAN_H
