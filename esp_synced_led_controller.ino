// Includes for FastLED controls
#define FASTLED_INTERNAL  // Supress pragma warnings about not using HW SPI for FastLED control
#include <FastLED.h>

// All the various animations
#include "animations/drop.h"
#include "animations/pulse.h"
#include "animations/rainbow.h"


// The Pin assignments for the various peripherals attached to the board.
static constexpr uint8_t kButtonPin = 0;
static constexpr uint8_t kSwitchPin = 14;
static constexpr uint8_t kLEDPin = 5;

// Debounce configurations for the buttons
constexpr int kBtnBounceTimeMS = 200;

// Details of the LEDs attached
static constexpr uint16_t kNumLEDs = 5; // How Many LEDs are connected
#define LED_TYPE WS2812B

// The baud rate to use for the serial output (debugging info/etc).
static constexpr uint32_t kSerialBaudRate = 115200;

// Stringification macros to convert a #defined constant into a string to
// output in a debugging message.
// Usage: "Serial.print("Macro: " xstring(MACRO));
#define xstr(a) str(a)
#define str(a) #a

// Animation settings
static constexpr uint16_t kNumFrames = 300; // How many frames each animation gets to run for (duration)
static constexpr uint8_t kFrameDelayMS = 20; // How long to delay (in MS) between generating frames

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
  static uint8_t next_animation_type = 0;
  // First, avoid memory leaks by deleting the animation that's ending.
  if (current_animation) {
    delete current_animation;
  }

  // Create a new animation object of the next type.
  current_animation = buildNewAnimation(
                          static_cast<AnimationType>(next_animation_type));
  next_animation_type = (next_animation_type + 1) % NUM_ANIMATION_TYPES;
}

// Interrupt handler for the brightness button.
// This works by setting the brightness_setting value with a new value.
// When the main loop sees that this value is changed, it'll update the
// brightness of the main LEDs via FastLEDs setBrightness() routine.
int brightnesses[] = {7, 128, 255};
int num_brightnesses = sizeof(brightnesses) / sizeof(brightnesses[0]);
volatile int brightness_setting = 0; // Start at he lowest brightness on boot
int current_brightness = brightness_setting;
static unsigned long last_brightness_press_time = 0;
void onBrightnessButtonChange() {
  // Handle a button press of the brightness button.
  unsigned long now = millis();
  if (now - last_brightness_press_time > kBtnBounceTimeMS &&
     !digitalRead(kButtonPin)) {
    brightness_setting = (brightness_setting + 1) % num_brightnesses;
  }
  last_brightness_press_time = now;
}

void setupUI() {
  // Set up the GPIOs/interrupts etc for all the user-facing interfaces such
  // as brightness controls and radio switches.

  // Set up the brightness button.
  pinMode(kButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(kButtonPin), onBrightnessButtonChange, CHANGE);
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
  Serial.println(brightnesses[brightness_setting]);
  pinMode(kLEDPin, OUTPUT);
  FastLED.addLeds<LED_TYPE, kLEDPin, RGB>(leds, kNumLEDs);
  FastLED.setBrightness(brightnesses[brightness_setting]);
  fill_solid(leds, kNumLEDs, CRGB::Black);
  FastLED.show();
}

void setup() {
  Serial.begin(kSerialBaudRate);
  Serial.println();
  Serial.println("--------------------------------------------------");
  Serial.println("ESP8266 Booting now.");
  Serial.println("--------------------------------------------------");

  setupUI();

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

    // Update FastLED's brightness setting if the user changed it.
    if (brightness_setting != current_brightness) {
      FastLED.setBrightness(brightnesses[brightness_setting]);
      current_brightness = brightness_setting;
    }
  }
}
