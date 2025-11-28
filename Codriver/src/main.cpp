#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "Backgrounds.hpp"

// pin dello schermetto

#define TFT_MOSI 11
#define TFT_SCLK 12  
#define TFT_DC   6
#define TFT_CS   10
#define TFT_RST  7

// per disegnare la lancetta

#define CENTER_X 120
#define CENTER_Y 120
#define LENGTH 70 // Lunghezza della linea

int deg = 0;

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0);

void drawLineFromCenter(float degrees, int length, int thickness, uint16_t color) {
    int centerX = 120;
    int centerY = 120;
    
    float rad = degrees * M_PI / 180.0;
    float cosR = cos(rad);
    float sinR = sin(rad);
    
    float halfT = thickness * 0.5;
    
    // Calcola i 4 angoli del rettangolo
    // Parte dal centro (0) e si estende fino alla lunghezza
    int x[4], y[4];
    
    // Punto iniziale (centro) - lato "sinistro" del rettangolo
    x[0] = centerX + (-halfT * sinR);
    y[0] = centerY + (halfT * cosR);
    
    // Punto iniziale (centro) - lato "destro" del rettangolo  
    x[1] = centerX + (halfT * sinR);
    y[1] = centerY + (-halfT * cosR);
    
    // Punto finale - lato "destro" del rettangolo
    x[2] = centerX + (length * cosR + halfT * sinR);
    y[2] = centerY + (length * sinR - halfT * cosR);
    
    // Punto finale - lato "sinistro" del rettangolo
    x[3] = centerX + (length * cosR - halfT * sinR);
    y[3] = centerY + (length * sinR + halfT * cosR);
    
    // Disegna due triangoli per formare il rettangolo
    gfx->fillTriangle(x[0], y[0], x[1], y[1], x[2], y[2], color);
    gfx->fillTriangle(x[0], y[0], x[2], y[2], x[3], y[3], color);
}



void setup() {
    Serial.begin(115200);
    gfx->begin();
    gfx->invertDisplay(true);  // Fix colori invertiti
    
    
    gfx->fillScreen(BLACK);
    
    Serial.println("✅ Bitmap disegnata!");
}

void loop() {
  //ridisegnamo per ora solamente sfondo e bitmap, da ottimizzare
  

  gfx->draw16bitBeRGBBitmap(0, 0, boost_bitmap, 240, 240);  // x, y, bitmap, width, height
    
  drawLineFromCenter(deg, LENGTH, 5, RED);
  deg+=10;
  delay(100);
}