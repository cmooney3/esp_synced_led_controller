#define FASTLED_INTERNAL  // Supress pragma warnings about not using HW SPI for FastLED control
#include <FastLED.h>

#define DELAY_MS 100

#define NUM_LEDS 1
#define LED_PIN 5
CRGB leds[NUM_LEDS];

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(64);
}

uint8_t h = 0;
int next_led_state = false;

// the loop function runs over and over again forever
void loop() {
  leds[0] = CHSV(h, 255, 128);
  h += 3;
  FastLED.show();
  
  digitalWrite(LED_BUILTIN, next_led_state);
  next_led_state = !next_led_state;

  delay(DELAY_MS); 
}
