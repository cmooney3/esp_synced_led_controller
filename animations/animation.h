#ifndef ANIMATION_H
#define ANIMATION_H

#include <FastLED.h>

constexpr int ANIMATION_DURATION_MS = 6000;

constexpr int US_PER_MS = 1000;
constexpr int US_PER_S = 1000000;

#define TO_MS(x) (x / US_PER_MS)

typedef struct AnimationInputs {
    CRGB* leds; // The FastLED struct containing the LED colors.
    uint16_t num_leds; // How many LEDs there are to control.

    uint32_t raw_time_us; // The current, raw network time (in microseconds).
    uint32_t time_since_animation_start_us; // The time since this animation started (in microseconds).

    uint32_t animation_number;  // Count of how many animations since the start have played.

    uint16_t num_controllers; // How many controllers are on the current network.
    uint16_t controller_number; // For the current network, what's this controller called.
} AnimationInputs;

int frame_number(const AnimationInputs& inputs, int frame_duration_ms) {
    return TO_MS(inputs.time_since_animation_start_us) / frame_duration_ms;
}

int odometer_signal(int base_signal, int prescalar, int range) {
    return (base_signal / prescalar) % range;
}

int cylon_signal(int base_signal, int prescalar, int range) {
    int adjusted_range = range * 2 - 2;
    int signal = odometer_signal(base_signal, prescalar, adjusted_range);
    if (signal >= range) {
        signal = adjusted_range - signal;
    }
    return signal;
}

CRGB random_color(const AnimationInputs& input, int color_index) {
    // Always start by re-seeding with the animation number, this means that you'll always get
    // the same random colors regardless of which frame you're rendering or which unit you are.
    srand(input.animation_number);
    // Next consume "color_index" random values to get to the "color_index"-th random number
    for (int i = 0; i < color_index; i++) {
        rand();  // Just consume them, drop the results...
    }
    int h = rand() % 255;
    return CHSV(h, 255, 255);
}

#endif  // ANIMATION_H
