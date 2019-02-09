#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
const char* wifi_ssid = "AirCanadaJazz";
const char* wifi_password = "xxxxxxxx";

// Here lies the code that sets the controller into OTA mode. This means it has
// to start up Wifi, and then sit there spinning waiting for an OTA
// TODO: add a timeout and go back to normal mode after a while

bool setupWifi() {
  // Connect to the wifi access point configured at the top of this file.
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("ERROR: Unable to connect to wifi.");
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void waitForOTA() {
  // This function sets up ArduinoOTA and then waits for incoming OTA updates
  // forever.

  // Set up callbacks in ArduinoOTA to display debugging information on the
  // the serial terminal before we get started.
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
    else Serial.println("Mystery Error");
  });

  // Start the OTA module up.
  ArduinoOTA.begin();

  // Wait forever until an OTA update arrives.
  while (true) {
    // Without this, the Watchdog resets the chip.  I'm not sure what's setting
    // it up.  Maybe because I'm not going into the loop()
    ESP.wdtFeed();

    Serial.println(WiFi.localIP());
    ArduinoOTA.handle();
  }
}

void enter_ota_mode() {
  // Start up wifi instead and connect to the access point.
  setupWifi();

  // Set up OTA and wait indefinitely doing nothing but checking for OTA updates.
  waitForOTA();
}
