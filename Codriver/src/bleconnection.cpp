
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
  int index = doc["bm"] | 0; 
  
  if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      ui_color = new_color;
      saveState("color", ui_color);
      saveState("screen", ui_index);

      if (ui_index < 0 || ui_index >= TOTAL_BITMAPS) {
        Serial.printf("ERRORE: Indice bitmap non valido: %d\n", index);
        return;
      } else {
        ui_index = index; 
      }
      
      xSemaphoreGive(xUIMutex); 
      changeBitmap();
  } else {
      Serial.println("❌ BLE: Mutex Timeout. Impossibile aggiornare UI.");
  }
}