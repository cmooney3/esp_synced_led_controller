#ifndef SEQUENCED_BLINK_H
#define SEQUENCED_BLINK_H

#include "animation.h"

constexpr static int kFrameDurationMS = 500;

void squencedBlinkAnimation(const AnimationInputs& inputs) {
    int frame = frame_number(inputs, kFrameDurationMS);
    uint16_t selected_controller = odometer_signal(frame, 1, inputs.num_controllers);

    CRGB fill_color = CRGB(0, 0, 0);
    if (selected_controller == inputs.this_controller) {
        fill_color = random_color(inputs, 0);
    }

    fill_solid(inputs.leds, inputs.num_leds, fill_color);
}

#endif // SEQUENCED_BLINK_H
