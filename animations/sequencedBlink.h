#ifndef SEQUENCED_BLINK_H
#define SEQUENCED_BLINK_H

#include "animation.h"

constexpr static int kFrameDurationMS = 250;

void squencedBlinkAnimation(const AnimationInputs& inputs) {
  int frame = frame_number(inputs, kFrameDurationMS);

  int selected_controller = odometer_signal(frame, 1, inputs.num_controllers);

  CRGB fill_color = (selected_controller == inputs.this_controller) ?
                        random_color(inputs, 0) : CRGB::Black;

  fill_solid(inputs.leds, inputs.num_leds, fill_color);
}

#endif // SEQUENCED_BLINK_H
