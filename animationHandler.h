#include "animations/animation.h"

// All the various animations themselves
#include "animations/scan.h"
#include "animations/rainbowFade.h"
#include "animations/rainbowScan.h"
#include "animations/sequencedBlink.h"

#define CHIP_ID_NOT_FOUND 0xFFFF

// These are defined in esp_synced_led_controller
extern CRGB leds[kNumLEDs];
extern painlessMesh mesh;

// Here we define the list of animations that the controller can play
typedef void (*AnimationFunction)(const AnimationInputs&);
AnimationFunction animations[] = {
    rainbowScanAnimation,
    scanAnimation,
    rainbowFadeAnimation,
    squencedBlinkAnimation,
};
constexpr uint8_t NUM_ANIMATIONS = sizeof(animations) / sizeof(animations[0]);

void fillAnimationInputs(AnimationInputs* inputs) {
    // Basic functionality -- you need the LED structure and the number of LEDs to render any animation
    inputs->leds = leds;
    inputs->num_leds = kNumLEDs;

    // Simply copy the raw network time (in us) into the struct in case the animation wants it.
    inputs->raw_time_us = mesh.getNodeTime();

    // Compute the animation number based on the time
    inputs->animation_number = TO_MS(inputs->raw_time_us) / ANIMATION_DURATION_MS;

    // Compute the the time (in us) since the start of this animation
    int animation_start_time_us = inputs->animation_number * ANIMATION_DURATION_MS * US_PER_MS;
    inputs->time_since_animation_start_us = inputs->raw_time_us - animation_start_time_us;


    // Get a list of all the chip ids on the network and sort it.  We need this information to determine
    // the controller-specific information that depends on the state of the network below.
    std::list<uint32_t> chip_ids = mesh.getNodeList();
    chip_ids.push_back(mesh.getNodeId());

    // The number of controllers currently in the mesh is just a count of how many chip IDs there are.
    inputs->num_controllers = chip_ids.size();

    // To determine this controllers "controller number" we simply sort the list of chip ids and see where
    // this controller's chip id falls in the list.  This provides a simple way to integrate per-controller
    // effects into your animations.
    chip_ids.sort();
    std::list<uint32_t>::const_iterator it;
    inputs->this_controller = CHIP_ID_NOT_FOUND;
    uint16_t i = 0;

    for (it = chip_ids.begin(); it != chip_ids.end(); it++) {
        if (*it == mesh.getNodeId()) {
            inputs->this_controller = i;
            break;
        } else {
            i++;
        }
    }

    if (inputs->this_controller == CHIP_ID_NOT_FOUND) {
        // This should _never_ happen -- this controller's chip ID should certainly be in the list
        // but just double-checking in case something goes screwy.
        Serial.println("ERROR: This controller's chip ID not found on it's own mesh!");
    }
}

void renderNextFrame() {
  AnimationInputs animation_inputs;
  fillAnimationInputs(&animation_inputs);
  int selected_animation = (TO_MS(animation_inputs.raw_time_us) / ANIMATION_DURATION_MS) % NUM_ANIMATIONS;
  animations[selected_animation](animation_inputs);
  FastLED.show();
}
