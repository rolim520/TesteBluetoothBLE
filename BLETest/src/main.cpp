#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// Define service UUID and characteristic UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID_TX "12345678-1234-1234-1234-123456789013"
#define CHARACTERISTIC_UUID_RX "12345678-1234-1234-1234-123456789014"

// Function to handle BLE events
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

// Function to handle data received from the central
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.print("Received JSON: ");
        Serial.println(rxValue.c_str());

        // Parse JSON
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, rxValue);

        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }

        // Access data from the JSON object
        const char* message = doc["message"];
        int number = doc["number"];

        Serial.print("Message: ");
        Serial.println(message);
        Serial.print("Number: ");
        Serial.println(number);
      }
    }
};

void setup() {
  Serial.begin(9600);

  // Create BLE device and server
  BLEDevice::init("ESP32_JSON_BLE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create TX characteristic
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Create RX characteristic
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                                          CHARACTERISTIC_UUID_RX,
                                          BLECharacteristic::PROPERTY_WRITE
                                        );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection...");
}

void loop() {
  // Notify connected device with a JSON message
  if (deviceConnected) {
    // Create JSON object
    StaticJsonDocument<200> doc;
    doc["message"] = "Hello from ESP32";
    doc["number"] = value;
    value++;

    // Serialize the JSON object to a string
    char buffer[128];
    serializeJson(doc, buffer);

    // Send the JSON string as a notification
    pTxCharacteristic->setValue(buffer);
    pTxCharacteristic->notify();
    delay(10); // Wait for 1 second between sends
  }

  // Handle device connection changes
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give time for the client to disconnect
    pServer->startAdvertising(); // Restart advertising
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    // Just connected
    oldDeviceConnected = deviceConnected;
  }
}
