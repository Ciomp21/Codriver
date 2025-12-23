
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "screen.hpp"
#include "global.hpp"

void parseJson(const char* jsonStr);

#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdef01-1234-5678-1234-56789abcdef0"

class ConfigCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    static char jsonBuffer[128];
    uint8_t* data = pCharacteristic->getData();
    size_t length = pCharacteristic->getValue().length();
    Serial.println("messaggio valido");
    //copio localmente il buffer della risposta
    if (length > 0 && length < 128) {
        memcpy(jsonBuffer, data, length);
        jsonBuffer[length] = '\0';
        parseJson(jsonBuffer);
    }
  }
};

void setupBLE() {  
    BLEDevice::init("Codriver");
    BLEDevice::setMTU(512);
    BLEServer *server = BLEDevice::createServer();
    BLEService *service = server->createService(SERVICE_UUID);
    BLECharacteristic *characteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );

    characteristic->setCallbacks(new ConfigCallback());
    service->start();
    server->getAdvertising()->start();
    Serial.println("BLE attivo. In attesa di messaggi");
}

void parseJson(const char* jsonStr) {
  Serial.println("BLE: Messaggio ricevuto:");
  Serial.println(jsonStr);
  
  StaticJsonDocument<128> doc; 
  DeserializationError error = deserializeJson(doc, jsonStr);
  if (error) {
    Serial.printf("JSON Errore: %s\n", error.f_str());
    return;
  }

  int r = doc["r"] | 0;
  int g = doc["g"] | 0;
  int b = doc["b"] | 0;
  uint16_t new_color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  int index = doc["index"] | 0;

  int min = doc["min"] | 0;
  int max = doc["max"] | 0;

  Serial.printf("Colore r: %d, g: %d, b: %d", r,g,b);
  Serial.printf("min: %d, max: %d\n", min, max);
  Serial.printf("index: %d\n", index);
   
  if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE) {

    if (ui_index < 0 || ui_index >= TOTAL_BITMAPS) {
        Serial.printf("ERRORE: Indice bitmap non valido: %d\n", index);
        xSemaphoreGive(xUIMutex);
        return;
    }

    Serial.println("Cambio UI");

    ui_color = new_color;
    ui_index = index;
    ui_update = true;
    OBDScreens[ui_index].max = max;
    OBDScreens[ui_index].min = min;
    xSemaphoreGive(xUIMutex);
  } else {
      Serial.println("❌ BLE: Mutex Timeout. Impossibile aggiornare UI.");
  }
}