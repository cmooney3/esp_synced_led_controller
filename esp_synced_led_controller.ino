// Includes for FastLED controls
#define FASTLED_INTERNAL  // Supress pragma warnings about not using HW SPI for FastLED control
#include <FastLED.h>

// Includes for ESP Arduino OTA FW updates
#include <ESP8266WiFi.h>
// Includes for ESP Arduino OTA FW updates
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Stringification macros to convert a #defined constant into a string to
// output in a debugging message.
// Usage: "Serial.print("Macro: " xstring(MACRO));
#define xstr(a) str(a)
#define str(a) #a

// Include a header file that defines the Wifi password.  This file is not
// included in the github repo (obviously) and needs to be added manually.
// The wifi_password.h file is also ignored by git (check .gitignore) to prevent
// Accidentally uploading wifi passwords to github.
// To use just create a file called password.h in this directory containing
// a single line like this:
// const char* wifi_password = "the_wifi_password";
#include "wifi_password.h"
extern const char* wifi_password;
const char* ssid = "AirCanadaJazz";

// Just like for the wifi password, it makes sense to keep this pwd off github.
// This loads the OTA password from a file called ota_password.h that you have
// to create with only one line like this:
// const char* ota_password = "the_ota_password";
#include "ota_password.h"
extern const char* ota_password;

#define NUM_LEDS 1
#define LED_PIN 5
#define LED_TYPE WS2812B
#define LED_BRIGHTNESS 64
CRGB leds[NUM_LEDS];

void setup() {
  // Serial setup for debugging purposes
  Serial.begin(115200);
  Serial.println();
  Serial.println("--------------------------------------------------");
  Serial.println("ESP8266 Booting now.");
  Serial.println("--------------------------------------------------");

  // Wifi Setup
  Serial.println("* Connecting to Wifi");
  Serial.print("  - SSID: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifi_password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("  - Connection FAILED! Rebooting ESP...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("  - IP address: ");
  Serial.println(WiFi.localIP());

  // Arduino OTA FW updating Setup
  Serial.println("* Setting up Arduino OTA FW updating");
  Serial.print("  - Password: \"");
  Serial.print(ota_password);
  Serial.println("\"");
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.onStart([]() { Serial.println("Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

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

  Serial.println("Set up complete!  Entering main program loop now.");
  Serial.println();
}

uint8_t h = 0;
void loop() {
  // Manage any Arduino OTA FW updates that might be waiting.
  ArduinoOTA.handle();

  // Update the LEDS to the next hue, creating a fading rainbow effect.
  fill_solid(leds, NUM_LEDS, CHSV(h++, 255, 128));
  FastLED.show();
  Serial.print("hue: ");
  Serial.println(h);

  // Delay a tiny bit before continuing so everything doesn't go too fast.
  delay(10);
}
