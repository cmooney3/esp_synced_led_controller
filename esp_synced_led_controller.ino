// Includes for FastLED controls
#define FASTLED_INTERNAL  // Supress pragma warnings about not using HW SPI for FastLED control
#include <FastLED.h>

// All the various animations
#include "animations/drop.h"
#include "animations/pulse.h"
#include "animations/rainbow.h"

// The baud rate to use for the serial output (debugging info/etc).
constexpr int kSerialBaudRate = 115200;

// Stringification macros to convert a #defined constant into a string to
// output in a debugging message.
// Usage: "Serial.print("Macro: " xstring(MACRO));
#define xstr(a) str(a)
#define str(a) #a

// How many frames each animation gets to run for (duration)
constexpr int kNumFrames = 300;

// How long to delay (in MS) between generating frames
constexpr int kFrameDelayMS = 20;

// How Many LEDs are connected
static constexpr int kNumLEDs = 5;

// Which pin (GPIO) are the LEDS connected to
static constexpr int kLEDPin = 5;

// Which brightness level should we run the LEDs at
static constexpr int kLEDBrightness = 64;

// Which type of LEDs are connected
#define LED_TYPE WS2812B

// Initialize the storage for all the LED values.  This is the format with which FastLED works.
CRGB leds[kNumLEDs];

// Here we define the list of animations that the controller can play
// by building up an enum full of their names and a generator function
// that returns a generic Animation* give the type of animation.
// When adding a new animation, this is where you do the book-keeping.
enum AnimationType {DROP, RAINBOW, PULSE, NUM_ANIMATION_TYPES};
Animation* buildNewAnimation(AnimationType type) {
  switch (type) {
    case AnimationType::PULSE:
      return new Pulse::PulseAnimation(leds, kNumLEDs, kNumFrames);
    case AnimationType::RAINBOW:
      return new Rainbow::RainbowAnimation(leds, kNumLEDs, kNumFrames);
    case AnimationType::DROP:
      return new Drop::DropAnimation(leds, kNumLEDs, kNumFrames);
    default:
      return NULL;
  }
}
Animation* current_animation;

void SwitchToNextAnimation() {
  static int next_animation_type = 0;
  // First, avoid memory leaks by deleting the animation that's ending.
  if (current_animation) {
    delete current_animation;
  }

  // Create a new animation object of the next type.
  current_animation = buildNewAnimation(
                          static_cast<AnimationType>(next_animation_type));
  next_animation_type = (next_animation_type + 1) % NUM_ANIMATION_TYPES;
}


void setupFastLED() {
  // LED setup for the RGB LEDs it's going to control
  Serial.println("* Configuring FastLED.");
  Serial.println("  - Type: " xstr(LED_TYPE));
  Serial.print("  - Pin: ");
  Serial.println(kLEDPin);
  Serial.print("  - Number of LEDs: ");
  Serial.println(kNumLEDs);
  Serial.print("  - Brightness (out of 255): ");
  Serial.println(kLEDBrightness);
  pinMode(kLEDPin, OUTPUT);
  FastLED.addLeds<LED_TYPE, kLEDPin, RGB>(leds, kNumLEDs);
  FastLED.setBrightness(kLEDBrightness);
  fill_solid(leds, kNumLEDs, CRGB::Black);
  FastLED.show();
}

void setup() {
  Serial.begin(kSerialBaudRate);
  Serial.println();
  Serial.println("--------------------------------------------------");
  Serial.println("ESP8266 Booting now.");
  Serial.println("--------------------------------------------------");

  setupFastLED();
  FastLED.setBrightness(16);

  // Set up the starting animation
  current_animation = buildNewAnimation(static_cast<AnimationType>(0));

  Serial.println("Set up complete!  Entering main program loop now.");
  Serial.println();
}

void loop() {
  SwitchToNextAnimation();

  bool has_more_frames = true;
  while(has_more_frames) {
    has_more_frames = current_animation->nextFrame();
    FastLED.show();
    FastLED.delay(kFrameDelayMS);
  }
}
