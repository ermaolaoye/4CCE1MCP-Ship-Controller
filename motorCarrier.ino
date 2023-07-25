#include <ArduinoMotorCarrier.h>

#include <ArduinoBLE.h>

// variables
volatile int16_t rotation = 0;
volatile int16_t udVal = 0;
volatile int16_t lrVal = 0;

static int duty = 0;
static int angle = 0;

void setup() {
  Serial.begin(9600);

  // BLE
  BLE.begin();

  Serial.println("BLE Initialized");

  BLE.scanForUuid("e36b42ba-d302-4e6b-9dbd-b48564acaa4d");

  // Motor Carrier
  controller.begin();

  M1.setDuty(0);
  M2.setDuty(0);

  servo1.setAngle(0);
}

void loop() {
  // check if a peripheral has beenn discorvered
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    // discovered the peripheral
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" ");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();

    // prevent coincidence
    if (peripheral.localName() != "group-h-controller") {
      return;
    }

    // Stop scanning
    BLE.stopScan();

    motorControl(peripheral);

    // peripheral disconnected, start scanning again
    BLE.scanForUuid("e36b42ba-d302-4e6b-9dbd-b48564acaa4d");
  }
}

void motorControl(BLEDevice peripheral) {
  Serial.println("Connecting...");

  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // retrieve characteristics
  BLECharacteristic udCharacteristic = peripheral.characteristic("9e0a38ca-a81e-49d1-8f15-5c1b2e6e4abc");
  BLECharacteristic lrCharacteristic = peripheral.characteristic("76066fb7-6ed0-4995-9781-afec08910176");
  BLECharacteristic rotationCharacteristic = peripheral.characteristic("ac1326fe-ba8f-4f57-833f-2bc88be33bd5");

  if (!udCharacteristic | !lrCharacteristic | !rotationCharacteristic) {
    Serial.println("Characteristic list doesn't match");
    peripheral.disconnect();
    return;
  }

  udCharacteristic.subscribe();
  lrCharacteristic.subscribe();
  rotationCharacteristic.subscribe();

  while(peripheral.connected()) {
    
    byte buf[4];

    if (udCharacteristic.valueUpdated()){
      udCharacteristic.readValue(buf, 4);
      udVal = (int16_t)((buf[1] << 8) | buf[0]); // Convert Bytes to integer.
      duty = map(udVal, -512, 512, -100, 100);
      M1.setDuty(duty);
      M2.setDuty(duty);
    }

    if (lrCharacteristic.valueUpdated()){
      lrCharacteristic.readValue(buf,4);
      lrVal = (int16_t)((buf[1] << 8) | buf[0]);
    }

    if (rotationCharacteristic.valueUpdated()) {
      rotationCharacteristic.readValue(buf,4);
      rotation = (int16_t)((buf[1] << 8) | buf[0]);

      servo1.setAngle(rotation);
    }

    // Debug Output
    Serial.print("UD = ");
    Serial.print(udVal, DEC);
    Serial.print(", LR = ");
    Serial.print(lrVal, DEC);
    Serial.print(", Rotation = ");
    Serial.print(rotation);
    Serial.print(", Duty = ");
    Serial.println(duty);
    
  }

  Serial.print("Connection Terminated");
}