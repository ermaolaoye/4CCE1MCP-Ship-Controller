#include <ArduinoBLE.h>

// define pin numbers for A, B, and button
#define A_PIN 2
#define B_PIN 21
#define BTN_PIN 20
#define UD_PIN A2
#define LR_PIN A3

#define LED_PIN LED_BUILTIN

#define ROTATION_LIMIT 45
#define JOYSTICK_DEADZONE 50

// variables
volatile int16_t rotation = 0;
volatile int16_t udAnalogReading = 0;
volatile int16_t udVal = 0;
volatile int16_t lrAnalogReading = 0;
volatile int16_t lrVal = 0;

const int16_t joystickDefaultVal = 512;

volatile bool sens;
volatile bool rotate;

long previousMillis = 0;

// BLE
BLEService controlService("e36b42ba-d302-4e6b-9dbd-b48564acaa4d");

BLEIntCharacteristic udCharacteristic("9e0a38ca-a81e-49d1-8f15-5c1b2e6e4abc", BLERead| BLENotify);
BLEIntCharacteristic lrCharacteristic("76066fb7-6ed0-4995-9781-afec08910176", BLERead| BLENotify);
BLEIntCharacteristic rotationCharacteristic("ac1326fe-ba8f-4f57-833f-2bc88be33bd5", BLERead| BLENotify);

void setup() {
  // initialize the serial port
  Serial.begin(19200);

  // set pin modes
  pinMode(A_PIN, INPUT);
  pinMode(B_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // attach interrupt service routines to interrupt pins
  attachInterrupt(digitalPinToInterrupt(A_PIN), isrA, FALLING);

  // BLE
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }

  BLE.setLocalName("group-h-controller");
  BLE.setAdvertisedService(controlService);

  controlService.addCharacteristic(udCharacteristic);
  controlService.addCharacteristic(lrCharacteristic);
  controlService.addCharacteristic(rotationCharacteristic);

  // add the service
  BLE.addService(controlService);

  udCharacteristic.writeValue(0);
  lrCharacteristic.writeValue(0);
  rotationCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
  // BLE
  BLEDevice central = BLE.central();

  if (central) {
    digitalWrite(LED_PIN, HIGH);

    while (central.connected()) {
      bleUpdate();
    }

    Serial.println("Lost Connection");                     
    digitalWrite(LED_PIN, LOW);
  }
}

// interrupt service routine for the A
void isrA() {
  if (digitalRead(A_PIN) == HIGH) {
    sens = digitalRead(B_PIN); // Clockwise rotation
  } else {
    sens = !digitalRead(B_PIN); // Anti-clockwise rotation
  }
  rotate = true;
}

void bleUpdate() {
  // Rotation
  if (rotate) {
    if (sens == HIGH) {
      rotation += 2;
    } else {
      rotation -= 2;
    }
    rotate = false;
  }

  if(digitalRead(BTN_PIN) == HIGH){
    rotation = 0; // Return to 0
  }

  // Joystick
  udAnalogReading = analogRead(UD_PIN);
  udVal = -(udAnalogReading - joystickDefaultVal); // Positive UP, Negative DOWN

  lrAnalogReading = analogRead(LR_PIN);
  lrVal = lrAnalogReading - joystickDefaultVal; // Positive RIGHT, Negative LEFT
  

  // Calibrate Rotation Limit
  if (rotation >= ROTATION_LIMIT) {
    rotation = ROTATION_LIMIT;
  } else if (rotation <= -ROTATION_LIMIT) {
    rotation = -ROTATION_LIMIT;
  }

  // Calibrate Joystick Deadzone (Along Different Axis)
  if (abs(udVal) <= JOYSTICK_DEADZONE) {
    udVal = 0;
  }
  if (abs(lrVal) <= JOYSTICK_DEADZONE) {
    lrVal = 0;
  }

  /* Calibrate Joystick Deadzone (Only Centre)
  if (abs(udVal) <= JOYSTICK_DEADZONE && abs(lrVal) <= JOYSTICK_DEADZONE) {
    udVal = 0;
    lrVal = 0;
  }
  */
  
  // BLE Update Value
  bool udChanged = (udCharacteristic.value() != udVal);
  bool lrChanged = (lrCharacteristic.value() != lrVal);
  bool rotationChanged = (rotationCharacteristic.value() != rotation);

  if (udChanged) {
    udCharacteristic.writeValue(udVal);
  }

  if (lrChanged) {
    lrCharacteristic.writeValue(lrVal);
  }

  if (rotationChanged) {
    rotationCharacteristic.writeValue(rotation);
  }

  // Debug Output
  Serial.print("UD = ");
  Serial.print(udVal, DEC);
  Serial.print(", LR = ");
  Serial.print(lrVal, DEC);
  Serial.print(", Rotation = ");
  Serial.println(rotation);
}