#include "animations/animation.h"

// All the various animations themselves
#include "animations/scan.h"
#include "animations/rainbowFade.h"
#include "animations/rainbowScan.h"

// These are defined in esp_synced_led_controller
extern CRGB leds[kNumLEDs];
extern painlessMesh mesh;

// This struct is filled with the inputs to the animations before each frame is rendered.
AnimationInputs animation_inputs;

// Here we define the list of animations that the controller can play
typedef void (*AnimationFunction)(const AnimationInputs&);
AnimationFunction animations[] = {
    rainbowScanAnimation,
    scanAnimation,
    rainbowFadeAnimation,
};
constexpr uint8_t NUM_ANIMATIONS = sizeof(animations) / sizeof(animations[0]);

void fillAnimationInputs(AnimationInputs* inputs) {
    animation_inputs.leds = leds;
    animation_inputs.num_leds = kNumLEDs;

    animation_inputs.raw_time_us = mesh.getNodeTime();

    animation_inputs.animation_number = TO_MS(animation_inputs.raw_time_us) / ANIMATION_DURATION_MS;

    int animation_start_time_us = animation_inputs.animation_number * ANIMATION_DURATION_MS * US_PER_MS;
    animation_inputs.time_since_animation_start_us =
        animation_inputs.raw_time_us - animation_start_time_us;
}

void renderNextFrame() {
  fillAnimationInputs(&animation_inputs);
  int selected_animation = (TO_MS(animation_inputs.raw_time_us) / ANIMATION_DURATION_MS) % NUM_ANIMATIONS;
  animations[selected_animation](animation_inputs);
  FastLED.show();
}
