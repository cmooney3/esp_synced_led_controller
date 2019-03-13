// EEPROM libraries so I can leave persistent notes across reboots
#include <EEPROM.h>
#define EEPROM_ADDR_OTA_MODE 0
#define OTA_MODE_ENABLED 0xD7
#define OTA_MODE_DISABLED 0

// Includes for FastLED controls
#define FASTLED_INTERNAL  // Supress pragma warnings about not using HW SPI for FastLED control
#include <FastLED.h>

// Includes/config for painlessMesh
#include <painlessMesh.h>
#define MESH_PORT 34553
#define MESH_PREFIX "esp_synced_leds_mesh"
extern const char* mesh_password;
#include "passwords/mesh_password.h"

String kProgrammingMsg = "PROG";

// Includes/config for Wifi
#include "ota.h"
bool is_ota_mode = false;

// The Pin assignments for the various peripherals attached to the board.
static constexpr uint8_t kButtonPin = 0;
static constexpr uint8_t kSwitchPin = 14;
static constexpr uint8_t kLEDPin = 5;

// When the unit is switched into programming mode how many times should it send out the
// programming trigger to all the other members of the mesh before rebooting itself
#define NUM_PROGRAMMING_BROADCASTS_BEFORE_REBOOT 20

// Debounce configurations for the buttons
constexpr int kBtnBounceTimeMS = 200;

// Details of the LEDs attached
constexpr uint16_t kNumLEDs = 5; // How Many LEDs are connected
#define LED_TYPE WS2812B

// The baud rate to use for the serial output (debugging info/etc).
constexpr uint32_t kSerialBaudRate = 115200;

// Stringification macros to convert a #defined constant into a string to
// output in a debugging message.
// Usage: "Serial.print("Macro: " xstring(MACRO));
#define xstr(a) str(a)
#define str(a) #a

// Initialize the storage for all the LED values.  This is the format with which FastLED works.
CRGB leds[kNumLEDs];

// All the animation stuff
#include "animationHandler.h"

// Create the actual mesh networking object for the mesh network.
painlessMesh mesh;

// Set up our scheduler and the tasks that the scheduler runs
Scheduler scheduler;
// The function declarations of the functions called by the tasks that we'll be scheduling
void sendMessage();
void renderNextFrame();
void checkSerial();
void updateBrightness();
void handleArduinoOTA();
void updateMesh();
void sendProgrammingMessge();
// The periods for each of these tasks in milliseconds
static constexpr uint8_t kFrameGenerationPeriodMS = 20;
static constexpr uint8_t kBrightnessUpdatePeriodMS = 250;
static constexpr uint16_t kSerialPollPeriodMS = 500;
static constexpr uint8_t kArduinoOTAPeriodMS = 20;
static constexpr uint8_t kMeshUpdatePeriodMS = 250;
static constexpr uint16_t kProgrammingTriggerPeriodMS = 2000;
// Build the actual "task" objects that are linked with periods, function pointers/etc
// These Tasks have to be added to the scheduler and enabled before they run, so not all
// of these tasks are enabled at once just because they appear here.
Task taskSendMessage(TASK_SECOND, TASK_FOREVER, &sendMessage);
Task taskRenderNextFrame(kFrameGenerationPeriodMS, TASK_FOREVER, &renderNextFrame);
Task taskUpdateBrightness(kBrightnessUpdatePeriodMS, TASK_FOREVER, &updateBrightness);
Task taskCheckSerial(kSerialPollPeriodMS, TASK_FOREVER, &checkSerial);
Task taskArduinoOTA(kArduinoOTAPeriodMS, TASK_FOREVER, &handleArduinoOTA);
Task taskUpdateMesh(kMeshUpdatePeriodMS, TASK_FOREVER, &updateMesh);
Task taskSendProgrammingMessage(kProgrammingTriggerPeriodMS, TASK_FOREVER, &sendProgrammingMessge);

// Interrupt handler for the brightness button.
// This works by setting the brightness_setting value with a new value.
// When the main loop sees that this value is changed, it'll update the
// brightness of the main LEDs via FastLEDs setBrightness() routine.
int brightnesses[] = {10, 128, 255};
int num_brightnesses = sizeof(brightnesses) / sizeof(brightnesses[0]);
volatile int brightness_setting = 2; // Start at he lowest brightness on boot
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
  // Configure FastLED for the main RGB LED strip that this unit controls
  Serial.println("* Configuring FastLED.");
  FastLED.addLeds<LED_TYPE, kLEDPin, RGB>(leds, kNumLEDs);

  // Set a starting brightness
  FastLED.setBrightness(brightnesses[brightness_setting]);

  // Set all the LEDs to black (aka off) so that the LEDs don't flash random colors
  // on boot
  fill_solid(leds, kNumLEDs, CRGB::Black);
  FastLED.show();
}

void handleIncomingMeshMessage(uint32_t from, String &msg) {
  Serial.printf("Received from %d msg=%s\n\r", from, msg.c_str());

  // Save a bit in the EEPROM and then reboot.  When we reboot the system will check the EEPROM
  // and boot into OTA mode.
  if (msg == kProgrammingMsg) {
    broadcastProgrammingMessagesBeforeRebootingIntoOTAMode();
  }
}

void sendProgrammingMessge() {
  static uint8_t sends_remaining = NUM_PROGRAMMING_BROADCASTS_BEFORE_REBOOT;
  Serial.print("Programming broadcast #");
  Serial.print(sends_remaining);
  Serial.print(": \"");
  Serial.print(kProgrammingMsg);
  Serial.println("\"");

  mesh.sendBroadcast(kProgrammingMsg);

  sends_remaining--;

  // After sending out all the messages, reboot yourself.
  if (sends_remaining == 0) {
    Serial.printf("Rebooting myself into OTA mode now.");
    ESP.restart();
  }
}

void newConnectionCallback(bool adopt) {
  Serial.printf("New Connection, adopt=%d\n\r", adopt);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections %s\n\r",mesh.subConnectionJson().c_str());
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n\r", mesh.getNodeTime(),offset);
}

void setupMeshNetworking() {
  // To set up the mesh network, you need to configure several callbacks.  This
  // function sets up those callbacks.

  // Configure which kinds of debugging messages PainlessMesh should show us on serial.
  // Note, we setDebugMsgTypes() before init() so that you can see startup messages.
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  // Availible Mesh debugging message types are:
  // ERROR MESH_STATUS CONNECTION SYNC COMMUNICATION GENERAL MSG_TYPES REMOTE

  // Initialize the mesh.
  mesh.init(MESH_PREFIX, mesh_password, &scheduler, MESH_PORT);

  // Configure the callbacks
  mesh.onReceive(&handleIncomingMeshMessage);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
}

void sendMessage() {
  // This is a stupid task that we won't really want in the end, but it's a good way to test
  // the mesh network/etc.  Hanging onto it for a bit until everything's working.
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
  Serial.printf("I just sent out \"%s\" as a broadcast!\n\r", msg.c_str());
  taskSendMessage.setInterval(random( TASK_SECOND * 1, TASK_SECOND * 5));
}

void broadcastProgrammingMessagesBeforeRebootingIntoOTAMode() {
  // Start this process by enabling  the programming message task.  It
  // will handle the rest of the steps itself.
  // This check makes sure that we only start the task one time.
  static bool has_started_broadcasting_already = false;
  if (!has_started_broadcasting_already) {
    Serial.print("Entering OTA Mode.  Broadcasting the message out first...");
    
    // Mark down in EEPROM that we should boot into OTA mode on the next reboot.
    EEPROM.write(EEPROM_ADDR_OTA_MODE, OTA_MODE_ENABLED);
    EEPROM.commit();

    // Enable a task to send out messages and eventually reboot this device.
    scheduler.addTask(taskSendProgrammingMessage);
    taskSendProgrammingMessage.enable();

    has_started_broadcasting_already = true;
  }
}

void checkSerial() {
  // Check to see if someone has sent the magic kProgrammingMsg to us over Serial.
  // It it has, broadcast the programming message out to everyone on the mesh network
  // and then reboot into OTA mode.
  if (Serial.available() > 0) {
    String input_str = Serial.readString();
    Serial.print("Serial Recieved: '");
    Serial.print(input_str);
    Serial.println("'!");

    if (input_str == kProgrammingMsg) {
      broadcastProgrammingMessagesBeforeRebootingIntoOTAMode();
    }
  }
}

void updateBrightness() {
  // Update FastLED's brightness setting if the user changed it.
  if (brightness_setting != current_brightness) {
    FastLED.setBrightness(brightnesses[brightness_setting]);
    current_brightness = brightness_setting;
  }
}

void handleArduinoOTA() {
  Serial.println("Handling ArduinoOTA...");
  ArduinoOTA.handle();
}

void updateMesh() {
  mesh.update();
}

void setup() {
  Serial.begin(kSerialBaudRate);
  Serial.println();
  Serial.println("--------------------------------------------------");
  Serial.println("ESP8266 Booting now.");
  Serial.println("--------------------------------------------------");

  // Enable use of the EEPROM
  EEPROM.begin(512);

  // Determine which mode we're booting in by read in a byte from the EEPROM.
  // If the byte at EEPROM_ADDR_OTA_MODE is set to OTA_MODE_ENABLED at boot,
  // then instead of the normal booting process we forgo everything apart from
  // connecting to wifi and waiting for OTA updates.
  uint8_t boot_mode_byte = EEPROM.read(EEPROM_ADDR_OTA_MODE);
  is_ota_mode = (boot_mode_byte == OTA_MODE_ENABLED);
  Serial.print("EEPROM Boot mode byte: 0x");
  Serial.print(boot_mode_byte, HEX);
  Serial.print("\t(Should enter OTA mode? ");
  Serial.print(is_ota_mode ? "yes" : "no");
  Serial.println(")");

  // Depending on which mode we're booting into, there are slightly different
  // steps for setting up.  Plus, each mode builds a different set of tasks to
  // run in our scheduler which really changes the behavior of each mode.
  if (is_ota_mode) {
    // If we're in here, then that means we're booting the system into OTA-mode

    // Make sure to change the eeprom OTA-mode switch off, so the next reboot
    // goes into regular-mode.
    EEPROM.write(EEPROM_ADDR_OTA_MODE, OTA_MODE_DISABLED);
    EEPROM.commit();

    // Start up standard Wifi (not a mesh network)
    setupWifi();

    // Set up the AndroidOTA callbacks and configure the library
    setupArduinoOTA();

    // Configure the tasks for OTA mode with our task scheduler.
    // For OTA mode there is basically only one thing we do -- just check for updates.
    // This simple task is the only one that will run in this mode, and essentially
    // just makes the main loop spin forever waiting on an update.
    scheduler.addTask(taskArduinoOTA);
    taskArduinoOTA.enable();
  } else {
    // If we're in here, then that means we're booting the system into the
    // normal mode where the device syncs on a mesh network and renders animations.

    // Do all the basic GPIO direction setting for buttons, switches, etc...
    setupUI();

    // Set up FastLED to control the actual LEDs.  This makes them enabled but
    // turns them all off, so it won't start as a big flash of random colors.
    setupFastLED();

    // Enable mesh networking.  When the mesh network is enabled this controller
    // can talk to the other nearby controllers.
    setupMeshNetworking();

    // Set up the periodic tasks for normal operation.
    // This task sends a periodic message -- used only to test/debug the mesh.
    scheduler.addTask(taskSendMessage);
    taskSendMessage.enable();
    // This task renders the actual frames of the animation.
    scheduler.addTask(taskRenderNextFrame);
    taskRenderNextFrame.enable();
    // This task polls the brightness button and updates the brightness.
    scheduler.addTask(taskUpdateBrightness);
    taskUpdateBrightness.enable();
    // This task polls the serial input looking for the "PROG" command.
    scheduler.addTask(taskCheckSerial);
    taskCheckSerial.enable();
    // Do the required mesh network maintenance.
    scheduler.addTask(taskUpdateMesh);
    taskUpdateMesh.enable();

    Serial.println("Set up complete!  Entering main program loop now.");
    Serial.println();
  }
}

void loop() {
  // The loop function should be handled entirely by the task scheduler.
  scheduler.execute();
}
