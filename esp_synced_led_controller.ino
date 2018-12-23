// Includes for FastLED controls
#define FASTLED_INTERNAL  // Supress pragma warnings about not using HW SPI for FastLED control
#include <FastLED.h>

#define SERIAL_BAUD_RATE 115200

// Stringification macros to convert a #defined constant into a string to
// output in a debugging message.
// Usage: "Serial.print("Macro: " xstring(MACRO));
#define xstr(a) str(a)
#define str(a) #a

#define NUM_LEDS 5
#define LED_PIN 5
#define LED_TYPE WS2812B
#define LED_BRIGHTNESS 64
CRGB leds[NUM_LEDS];

void setupFastLED() {
  // LED setup for the RGB LEDs it's going to control
  Serial.println("* Configuring FastLED.");
  Serial.println("  - Type: " xstr(LED_TYPE));
  Serial.print("  - Pin: ");
  Serial.println(LED_PIN);
  Serial.print("  - Number of LEDs: ");
  Serial.println(NUM_LEDS);
  Serial.print("  - Brightness (out of 255): ");
  Serial.println(LED_BRIGHTNESS);
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<LED_TYPE, LED_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println();
  Serial.println("--------------------------------------------------");
  Serial.println("ESP8266 Booting now.");
  Serial.println("--------------------------------------------------");

  setupFastLED();
  FastLED.setBrightness(16);

  Serial.println("Set up complete!  Entering main program loop now.");
  Serial.println();
}

uint8_t h = 0;
void loop() {
  // Update the LEDS to the next hue, creating a fading rainbow effect.
  fill_rainbow(leds, NUM_LEDS, h++, 10);

  FastLED.show();

  // Delay a tiny bit before continuing so everything doesn't go too fast.
  delay(10);
}
