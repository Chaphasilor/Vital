#include <Arduino.h>
#include <string>
#include <list>
#include <iostream>

std::list<int> meassurements; // define list

// char stringToBytes(std::string s) {
//   return s.c_str();
// }

// Bluetooth LE ---------------------------

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
bool deviceConnected = false;
BLEServer *pServer;
BLEService *pTempService;
BLECharacteristic *pTemp, *pTempType, *pLED;
float currentTempFloat;
int currentTempInt;

// BM(E/P)280 ---------------------------------

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp;



void updateTemperature() {
  currentTempFloat = bmp.readTemperature();
  Serial.println(currentTempFloat);
  currentTempInt = static_cast<int>(currentTempFloat * 100);
  pTemp->setValue(currentTempInt);
}

class ServerCallbacks: public BLEServerCallbacks { // server/connection event handlers
    void onConnect(BLEServer* pServer) {
      updateTemperature();
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class TempCallbacks: public BLECharacteristicCallbacks { // characteristic event handlers
    void onRead() {
      updateTemperature();
    }
};

class LEDCallbacks: public BLECharacteristicCallbacks { // characteristic event handlers
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value == "on") {
        digitalWrite(2, HIGH);
        Serial.println("LED is on");
      } else {
        digitalWrite(2, LOW);
        Serial.println("LED is off");
      }
    }
};



void setup() {
  Serial.begin(115200);

  meassurements = {}; // inti list

  // Bluetooth LE -----------------
  Serial.println("Starting BLE server!");

  BLEDevice::setPower(ESP_PWR_LVL_P7);
  BLEDevice::init("Vitals");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks()); // define event handling

  pTempService = pServer->createService(BLEUUID((uint16_t)0x1809));

  pTemp = pTempService->createCharacteristic(BLEUUID((uint16_t)0x2A6E), BLECharacteristic::PROPERTY_READ);
  pTemp->setCallbacks(new TempCallbacks());

  pTempType = pTempService->createCharacteristic(BLEUUID((uint16_t)0x2A1D), BLECharacteristic::PROPERTY_READ);

  pLED = pTempService->createCharacteristic(BLEUUID((uint16_t)0x25CD), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pLED->setCallbacks(new LEDCallbacks()); // define event handling

  int typeKey = 1; // Armpit
  pTempType->setValue(typeKey);
  int currentTemp = 0;
  pTemp->setValue(currentTemp);
  pLED->setValue("off");

  pTempService->addCharacteristic(pTemp);
  pTempService->addCharacteristic(pTempType);
  pTempService->addCharacteristic(pLED);
  pTempService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x1809));
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Server initialized successfully!");

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  // BME280 -----------------------------

  if (!bmp.begin(0x76)) {
    Serial.println("Sensor not found!");
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  std::string s("Emplary");
  Serial.println(s.c_str()); // use this to convert string to "byte array"
  updateTemperature();
  if (deviceConnected) {
    Serial.println("A client is currently connected");
    Serial.print("Current temperature: ");
    Serial.print(currentTempFloat);
    Serial.println("°C");
  } else {
    Serial.println("No client connected");
  }

  delay(2000);

}
