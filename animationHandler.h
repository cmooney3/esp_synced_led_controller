#include "animations/animation.h"

// All the various animations themselves
#include "animations/scan.h"
#include "animations/flash.h"

// These are defined in esp_synced_led_controller
extern CRGB leds[kNumLEDs];
extern painlessMesh mesh;


// Here we define the list of animations that the controller can play
typedef void (*AnimationFunction)(uint32_t, CRGB*, int);
AnimationFunction animations[] = {
    scanAnimation,
    flashAnimation,
};
constexpr uint8_t NUM_ANIMATIONS = sizeof(animations) / sizeof(animations[0]);

void renderNextFrame() {
  uint32_t time = mesh.getNodeTime();
  int selected_animation = (TO_MS(time) / ANIMATION_DURATION_MS) % NUM_ANIMATIONS;
  animations[selected_animation](time, leds, kNumLEDs);
  FastLED.show();
}
