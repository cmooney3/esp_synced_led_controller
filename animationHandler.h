// All the various animations themselves
#include "animations/scan.h"

// These are defined in esp_synced_led_controller
extern CRGB leds[kNumLEDs];
extern painlessMesh mesh;

// Here we define the list of animations that the controller can play
typedef void (*AnimationFunction)(uint32_t, CRGB*, int);
AnimationFunction animations[] = {
    scanAnimation,
};
constexpr uint8_t NUM_ANIMATIONS = sizeof(animations) / sizeof(animations[0]);

void renderNextFrame() {
  animations[0](mesh.getNodeTime(), leds, kNumLEDs);
  FastLED.show();
}
