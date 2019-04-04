# ifndef OTA_H
# define OTA_H

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "animations/animation.h"

#define OTAMODE_COLOR_ONE CRGB::Red
#define OTAMODE_COLOR_TWO CRGB::Green


const char* wifi_ssid = "AirCanadaJazz";
extern const char* wifi_password;
#include "passwords/wifi_password.h"

// Here lies the code that sets the controller into OTA mode. This means it has
// to start up Wifi, and then sit there spinning waiting for an OTA
// TODO: add a timeout and go back to normal mode after a while

bool setupWifi() {
  // Connect to the wifi access point configured at the top of this file.
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupArduinoOTA(CRGB* leds, int num_leds) {
  static CRGB* the_leds = leds;
  static int the_num_leds = num_leds;

  ArduinoOTA.onStart([]() {
    Serial.println("Starting update");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int percent_updated = progress / (total / 100);
    Serial.printf("Progress: %d\r", percent_updated);

    double fill_amount = ((double)percent_updated / 100.0) * the_num_leds;
    int fill_amount_full = (int)fill_amount;
    double remainder = fill_amount - (double)fill_amount_full;
    uint8_t blend_factor = remainder * 255.0;
    fill_solid(the_leds, the_num_leds, OTAMODE_COLOR_ONE);
    fill_solid(the_leds, fill_amount_full, OTAMODE_COLOR_TWO);
    the_leds[fill_amount_full] = blend(OTAMODE_COLOR_ONE, OTAMODE_COLOR_TWO, blend_factor);
    FastLED.show();
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
  Serial.println("ArduioOTA Ready!");
}

void OTAModeAnimation(CRGB* leds, int num_leds, uint32_t time_us) {
  uint8_t blend_factor = cylon_signal(TO_MS(time_us), 3, 255);
  CRGB color = blend(OTAMODE_COLOR_ONE, OTAMODE_COLOR_TWO, blend_factor);
  fill_solid(leds, num_leds, color);
  FastLED.show();
}

#endif // OTA_H
