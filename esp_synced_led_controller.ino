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

// How frequently should we be checking for new brightness levels
static constexpr uint8_t kBrightnessUpdateMS = 250;

// How long to wait in between checking the serial port for input
static constexpr uint8_t kSerialPollPeriodMS = 500;

// Initialize the storage for all the LED values.  This is the format with which FastLED works.
CRGB leds[kNumLEDs];

// Build out the Mesh networking object, the scheduler and the tasks the scheduler runs
painlessMesh mesh;
Scheduler userScheduler; // This is the main scheduler that schedules everything
// The function declarations of the functions called by the tasks that we'll be scheduling
void sendMessage();
void renderNextFrame();
void checkSerial();
void updateBrightness();
// Build the actual "task" objects that are linked with periods, function pointers/etc
Task taskSendMessage(TASK_SECOND, TASK_FOREVER, &sendMessage);
Task taskRenderNextFrame(kFrameDelayMS, TASK_FOREVER, &renderNextFrame);
Task taskUpdateBrightness(kBrightnessUpdateMS, TASK_FOREVER, &updateBrightness);
Task taskCheckSerial(kSerialPollPeriodMS, TASK_FOREVER, &checkSerial);


// Here we define the list of animations that the controller can play
// by building up an enum full of their names and a generator function
// that returns a generic Animation* give the type of animation.
// When adding a new animation, this is where you do the book-keeping.
enum AnimationType {ANIM_DROP, ANIM_RAINBOW, ANIM_PULSE, NUM_ANIMATION_TYPES} typedef AnimationType;
Animation* buildNewAnimation(AnimationType type) {
  switch (type) {
    case AnimationType::ANIM_PULSE:
      return new Pulse::PulseAnimation(leds, kNumLEDs, kNumFrames);
    case AnimationType::ANIM_RAINBOW:
      return new Rainbow::RainbowAnimation(leds, kNumLEDs, kNumFrames);
    case AnimationType::ANIM_DROP:
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
  Serial.printf("New animation Started (type: %d)\n\r", next_animation_type);

  // Advance to the next animation.
  next_animation_type = (next_animation_type + 1) % NUM_ANIMATION_TYPES;
}

// Interrupt handler for the brightness button.
// This works by setting the brightness_setting value with a new value.
// When the main loop sees that this value is changed, it'll update the
// brightness of the main LEDs via FastLEDs setBrightness() routine.
int brightnesses[] = {7, 10, 30};
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
  FastLED.addLeds<LED_TYPE, kLEDPin, RGB>(leds, kNumLEDs);

  FastLED.setBrightness(brightnesses[brightness_setting]);

  fill_solid(leds, kNumLEDs, CRGB::Black);
  FastLED.show();
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %d msg=%s\n\r", from, msg.c_str());

  // Save a bit in the EEPROM and then reboot.  When we reboot the system will check the EEPROM
  // and boot into OTA mode.
  if (msg == kProgrammingMsg) {
    Serial.printf("Programming command received.  Restarting into OTA mode.");
    EEPROM.write(EEPROM_ADDR_OTA_MODE, OTA_MODE_ENABLED);
    EEPROM.commit();
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
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, mesh_password, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
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

void renderNextFrame() {
  // Render the next frame
  bool has_more_frames = current_animation->nextFrame();
  FastLED.show();

  // If that was the last frame of the animation, queue up the next one
  if (!has_more_frames) {
    SwitchToNextAnimation();
  }
}

void checkSerial() {
  if (Serial.available() > 0) {
    String input_str = Serial.readString();

    Serial.print("Serial Recieved: '");
    Serial.print(input_str);
    Serial.println("'!");

    if (input_str == kProgrammingMsg) {
      Serial.print("Entering Program Mode.  Broadcasting for you now.");
      for (int i = 0; i < 10; i++) {
        // TODO: Send out the Programming message to everyone before resetting itself
        mesh.sendBroadcast(kProgrammingMsg);
      }

      // Super hacky -- this callback is the received callback so it acts like
      // it received the same message.
      receivedCallback(0, kProgrammingMsg);
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
void setupTasks() {
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  userScheduler.addTask(taskRenderNextFrame);
  taskRenderNextFrame.enable();

  userScheduler.addTask(taskUpdateBrightness);
  taskUpdateBrightness.enable();

  userScheduler.addTask(taskCheckSerial);
  taskCheckSerial.enable();
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
  if (is_ota_mode) {
    EEPROM.write(EEPROM_ADDR_OTA_MODE, OTA_MODE_DISABLED);
    EEPROM.commit();
    // Start up wifi instead and connect to the access point.
    setupWifi();

    // Set up OTA and wait indefinitely doing nothing but checking for OTA updates.
    setupArduinoOTA();
  } else {
    // If we're here, then we know that we are *not* in OTA mode, and thus
    // we should continue our setup as usual.

    // Do all the basic GPIO direction setting for buttons, switches, etc...
    setupUI();

    // Set up FastLED to control the actual LEDs.  This makes them enabled but
    // turns them all off, so it won't start as a big flash of random colors.
    setupFastLED();
    FastLED.setBrightness(16);

    // Enable mesh networking.  When the mesh network is enabled this controller
    // can talk to the other nearby controllers.
    setupMeshNetworking();

    // Set up the task scheduler with our periodic tasks.  These are the main
    // threads of operation.
    setupTasks();

    // Set up the starting animation  Just pick the first one in the list.
    current_animation = buildNewAnimation(static_cast<AnimationType>(0));

    Serial.println("Set up complete!  Entering main program loop now.");
    Serial.println();
  }
}

void loop() {
  if (is_ota_mode) {
    Serial.println("Handling ArduinoOTA...");
    ArduinoOTA.handle();
  } else {
    userScheduler.execute();
    mesh.update();
  }
}
