#include "animations/animation.h"

// All the various animations themselves
#include "animations/scan.h"
#include "animations/rainbowFade.h"
#include "animations/rainbowScan.h"

#define CHIP_ID_NOT_FOUND 0xFFFFFFFF

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
    // Basic functionality -- you need the LED structure and the number of LEDs to render any animation
    animation_inputs.leds = leds;
    animation_inputs.num_leds = kNumLEDs;

    // Simply copy the raw network time (in us) into the struct in case the animation wants it.
    animation_inputs.raw_time_us = mesh.getNodeTime();

    // Compute the animation number based on the time
    animation_inputs.animation_number = TO_MS(animation_inputs.raw_time_us) / ANIMATION_DURATION_MS;

    // Compute the the time (in us) since the start of this animation
    int animation_start_time_us = animation_inputs.animation_number * ANIMATION_DURATION_MS * US_PER_MS;
    animation_inputs.time_since_animation_start_us = animation_inputs.raw_time_us - animation_start_time_us;


    // Get a list of all the chip ids on the network and sort it.  We need this information to determine
    // the controller-specific information that depends on the state of the network below.
    std::list<uint32_t> chip_ids = mesh.getNodeList();

    // The number of controllers currently in the mesh is just a count of how many chip IDs there are.
    animation_inputs.num_controllers = chip_ids.size();

    // To determine this controllers "controller number" we simply sort the list of chip ids and see where
    // this controller's chip id falls in the list.  This provides a simple way to integrate per-controller
    // effects into your animations.
    chip_ids.sort();
    std::list<uint32_t>::const_iterator it;
    animation_inputs.controller_number = CHIP_ID_NOT_FOUND;
    uint32_t i = 0;
    for (it = chip_ids.begin(); it != chip_ids.end(); it++) {
        if (*it == mesh.getNodeId()) {
            animation_inputs.controller_number = i;
            break;
        } else {
            i++;
        }
    }
    if (animation_inputs.controller_number == CHIP_ID_NOT_FOUND) {
        // This should _never_ happen -- this controller's chip ID should certainly be in the list
        // but just double-checking in case something goes screwy.
        Serial.println("ERROR: This controller's chip ID not found on it's own mesh!");
    }
}

void renderNextFrame() {
  fillAnimationInputs(&animation_inputs);
  int selected_animation = (TO_MS(animation_inputs.raw_time_us) / ANIMATION_DURATION_MS) % NUM_ANIMATIONS;
  animations[selected_animation](animation_inputs);
  FastLED.show();
}
