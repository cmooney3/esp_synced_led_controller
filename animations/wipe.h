#ifndef WIPE_H
#define WIPE_H

#include "animation.h"

#define WIPE_RATE_HZ 7
#define US_PER_TICK (US_PER_S / WIPE_RATE_HZ)

#define WIPE_FRAME_DURATION_MS 20


void wipeAnimation(const AnimationInputs& inputs) {
  int frame = frame_number(inputs, WIPE_FRAME_DURATION_MS);

  int split_point = odometer_signal(frame, 1, inputs.num_leds);

  int fill_color_index = odometer_signal(frame, inputs.num_leds, 2);
  int other_color_index = (fill_color_index == 0) ? 1 : 0;

  fill_solid(inputs.leds, split_point, random_color(inputs, fill_color_index));
  fill_solid(&inputs.leds[split_point], inputs.num_leds - split_point,
             random_color(inputs, other_color_index));
}

#endif // WIPE_H
