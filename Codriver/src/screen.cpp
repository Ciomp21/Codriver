#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// questi servono per agire sul filesystem
#include <LittleFS.h>
#include <FS.h>

#include "global.hpp"

// pin dello schermetto
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS 10
#define TFT_DC 6
#define TFT_RST 7
// variabili linea
#define CENTER_X 120
#define CENTER_Y 120
#define LENGTH 70
// filesystem
#define LINE_BUFFER_SIZE (240 * 2)

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0);

float deg = 0;
float target_deg = 0;
float current_deg = 170.0;
float smooth = 0.1;
float val = 0;
int decimals = 2;
float current_min = 0.0;
float current_max = 100.0;

static uint16_t lineBuffer[240];
void changeBitmap(int index);

// Setting up the screen and loading the states from the last shutdown
void setupScreen()
{

  gfx->begin();
  gfx->invertDisplay(true);
  gfx->fillScreen(RED);

  // inizializzo qui il sistema
  if (!LittleFS.begin(true))
  {
    Serial.println("❌ Errore critico: Impossibile inizializzare LittleFS!");
    while (true)
    {
      delay(100);
    };
  }

  if (xSemaphoreTake(xUIMutex, portMAX_DELAY) == pdTRUE)
  {
    ui_index = 0;
    ui_color = 0xFFFFFF;
    err = 0;
    xSemaphoreGive(xUIMutex);
  }

  // sets the loading bitmap with the logo
  changeBitmap(0);

  // wait for the wifi to be connected before loading the states, otherwise we might have problems with the preferences library
  // while (!is_tcp_connected)
  // {
  //   vTaskDelay(pdMS_TO_TICKS(100));
  // }

  if (xSemaphoreTake(xUIMutex, portMAX_DELAY) == pdTRUE)
  {
    // just for testing
    ui_color = loadState("color");
    ui_index = loadState("screen");
    float min = loadState("min");
    float max = loadState("max");
    ui_update = true;

    Serial.printf("Loaded states: color: %d, screen: %d, min: %f, max: %f\n", ui_color, ui_index, min, max);

    if (min != -1)
    {
      OBDScreens[ui_index].min = min;
    }
    if (max != -1)
    {
      OBDScreens[ui_index].max = max;
    }
    err = 0;
    xSemaphoreGive(xUIMutex);
  }
}

// we use this to change the bitmap being displayed
// the bitmaps are stored in LittleFS
// in order to add a new bitmap just turn it into a .bin file
// with 240x240 pixels and upload it to LittleFS with the appropriate tool

// you also neet to add its path and parameters to the OBDScreens array in global.cpp
// and load the fs with platiformio tool

void changeBitmap(int index)
{
  int current_index = index;

  Serial.printf("Preso file: %s\n", OBDScreens[current_index].bitmap_file);
  // load the brtmap from LittleFS
  File file = LittleFS.open(OBDScreens[current_index].bitmap_file, "r");
  if (!file)
  {
    Serial.printf("ERRORE: Impossibile aprire il file: %s\n", OBDScreens[current_index].bitmap_file);
    return;
  }

  gfx->fillScreen(BLACK);

  // we draw a screen only if it has a draw function
  if (OBDScreens[current_index].drawFunction)
  {
    for (int16_t i = 0; i < 240; i++)
    {
      file.readBytes((char *)lineBuffer, LINE_BUFFER_SIZE);
      gfx->draw16bitBeRGBBitmap(0, i, lineBuffer, 240, 1);
    }
  }

  file.close();
}

float toAngle(float value, float minVal, float maxVal, float startAngle, float sweepAngle)
{
  if (maxVal - minVal == 0)
    return startAngle;
  if (value < minVal)
    value = minVal;
  if (value > maxVal)
    value = maxVal;

  float percent = (value - minVal) / (maxVal - minVal);
  return startAngle + percent * sweepAngle;
}

void drawLineFromCenter(float degrees, int length, int thickness, int color)
{
  int centerX = 120;
  int centerY = 120;

  float rad = degrees * M_PI / 180.0;
  float cosR = cos(rad);
  float sinR = sin(rad);

  float halfT = thickness * 0.5;

  int x[4], y[4];

  x[0] = centerX + (-halfT * sinR);
  y[0] = centerY + (halfT * cosR);

  x[1] = centerX + (halfT * sinR);
  y[1] = centerY + (-halfT * cosR);

  x[2] = centerX + (length * cosR + halfT * sinR);
  y[2] = centerY + (length * sinR - halfT * cosR);

  x[3] = centerX + (length * cosR - halfT * sinR);
  y[3] = centerY + (length * sinR + halfT * cosR);

  gfx->fillTriangle(x[0], y[0], x[1], y[1], x[2], y[2], color);
  gfx->fillTriangle(x[0], y[0], x[2], y[2], x[3], y[3], color);
}

void drawGauge(float value)
{
  // Serial.println(value);
  if (value == -1)
  {
    value = val;
  }

  // smoothing
  target_deg = toAngle(value, current_min, current_max, 170.0, 200.0);
  current_deg = current_deg + (target_deg - current_deg) * smooth;

  int current_color;
  if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    current_color = ui_color;
    xSemaphoreGive(xUIMutex);
  }
  else
  {
    current_color = ui_color;
  }

  drawLineFromCenter(deg, LENGTH, 5, BLACK);
  drawLineFromCenter(current_deg, LENGTH, 5, current_color);

  deg = current_deg;
}

void drawRotatedLine(int cx, int cy, float x1, float y1, float x2, float y2, float s, float c)
{
  int rx1 = cx + x1 * c - y1 * s;
  int ry1 = cy + x1 * s + y1 * c;

  int rx2 = cx + x2 * c - y2 * s;
  int ry2 = cy + x2 * s + y2 * c;

  gfx->drawLine(rx1, ry1, rx2, ry2, WHITE);
}

void drawScreen()
{
  int current_index;
  bool update = false;

  if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    current_index = ui_index;

    if (ui_update)
    {
      update = true;
      ui_update = false;
    }
    xSemaphoreGive(xUIMutex);
  }
  else
  {
    current_index = ui_index;
  }

  DataTypes_t screen = OBDScreens[current_index];

  if (update)
  {
    if (getError() != 0)
    {
      changeBitmap(0);
    }
    else
    {
      Serial.printf("Updating UI! index: %d\n", current_index);
      changeBitmap(current_index);
      current_min = screen.min;
      current_max = screen.max;
    }
  }

  if (getError() != 0)
  {
    OBDScreens[0].drawFunction();
    return;
  }

  if (screen.drawFunction)
  {
    screen.drawFunction();
  }
}

// here u can add new draw functions for the various screens

float boostValues[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.75, 1.7, 1.65, 1.6, 1.55, 1.5, 1.45, 1.4, 1.35, 1.3, 1.25, 1.2, 1.15, 1.1, 1.05, 1.0};
int boostIndex = 0;

void drawBoost()
{
  float boostValue = boostValues[boostIndex++ % (sizeof(boostValues) / sizeof(boostValues[0]))];

  // float boostValue;
  // if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  // {
  //   boostValue = liveData.boost;
  //   // important to set those values to -1 after reading
  //   // for easy smoothing
  //   liveData.boost = -1.0;
  //   xSemaphoreGive(xDataMutex);
  // }
  // else
  // {
  //   boostValue = liveData.boost;
  // }

  // Serial.printf("boost: %f, min: %f, max: %f\n", boostValue, current_min, current_max);

  // i get the length of the string
  int length = snprintf(NULL, 0, "%.*f", decimals, boostValue);
  int prevlength = snprintf(NULL, 0, "%.*f", decimals, val);

  drawGauge(boostValue);

  if (boostValue == -1.0)
  {
    return;
  }

  int startX = 120 - (length * 18) / 2;
  int prevStartX = 120 - (prevlength * 18) / 2;
  // da vedere questo
  if (abs(boostValue - val) > 0.001)
  {
    // cancello
    gfx->setTextSize(3);
    gfx->setCursor(prevStartX, 150);
    gfx->setTextColor(BLACK);
    gfx->print(val, 2);
  }

  val = boostValue;

  gfx->setTextSize(3);
  gfx->setCursor(startX, 150);
  gfx->setTextColor(WHITE);
  gfx->print(boostValue, 2);
}

void drawRPM()
{
  float value;
  if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    value = liveData.rpm;
    // important to set those values to -1 after reading
    // for easy smoothing
    liveData.rpm = -1.0;
    xSemaphoreGive(xDataMutex);
  }
  else
  {
    value = liveData.rpm;
  }

  // Serial.print("rpm: ");
  // Serial.println(value);

  int length = snprintf(NULL, 0, "%.*f", decimals, value);

  drawGauge(value);

  int startX = 120 - (length * 18) / 2;
  if (value == -1)
  {
    return;
  }
  // da vedere questo
  if (abs(value - val) > 0.001)
  {
    // cancello
    gfx->setTextSize(3);
    gfx->setCursor(startX, 150);
    gfx->setTextColor(BLACK);
    gfx->print(startX, decimals);
  }

  val = value;

  gfx->setTextSize(3);
  gfx->setCursor(110, 150);
  gfx->setTextColor(WHITE);
  gfx->print(value, decimals);
}

// Test function to draw the acceleration values on the screen, to be used for testing the IMU sensor and the G-force calculations
float ax[] = {0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1,
              1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1,
              2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0, 3.1,
              3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0, 4.1,
              4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8};
float ay[] = {0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1,
              1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1,
              2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0, 3.1,
              3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0, 4.1,
              4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8};
float az[] = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
              1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9,
              2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9,
              3.0, 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9,
              4.0, 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8};
int accelIndex = 0;

void drawAcceleration()
{
  float oldax = 0, olday = 0, oldaz = 0;
  float ax, ay, az;

  // get the previous values in case of change
  oldax = liveData.accelX;
  olday = liveData.accelY;
  oldaz = liveData.accelZ;

  // if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  // {
  //   // these are the new values to read
  //   ax = liveData.accelX;
  //   ay = liveData.accelY;
  //   az = liveData.accelZ;

  //   // important to set those values to -1 after reading
  //   // for easy smoothing
  //   liveData.accelX = -1.0;
  //   liveData.accelY = -1.0;
  //   liveData.accelZ = -1.0;
  //   xSemaphoreGive(xDataMutex);
  // }
  // else
  // {
  //   ax = liveData.accelX;
  //   ay = liveData.accelY;
  //   az = liveData.accelZ;
  // }

  // cancello
  gfx->setTextSize(3);
  gfx->setCursor(50, 100);
  gfx->setTextColor(BLACK);
  gfx->print("X:");
  gfx->print(oldax);
  gfx->setCursor(50, 140);
  gfx->print("Y:");
  gfx->print(olday);
  gfx->setCursor(50, 180);
  gfx->print("Z:");
  gfx->print(oldaz);

  // scrivo
  gfx->setTextSize(3);
  gfx->setCursor(50, 100);
  gfx->setTextColor(WHITE);
  gfx->print("X:");
  gfx->print(ax);
  gfx->setCursor(50, 140);
  gfx->print("Y:");
  gfx->print(ay);
  gfx->setCursor(50, 180);
  gfx->print("Z:");
  gfx->print(az);
}

// Test function to draw a rectangle rotated by an angle, to be used for the front view of the car
int rotIndex = 0;
float rotAngles[] = {1.3, 1.6, 1.8, 1.9, 2.1, 2.1, 3.2, 3.4, 3.5, 4.1, 4.4, 4.8, 5, 5.2, 7.1, 7.4, 7.5, 8, 8.4, 9.1, 9.2, 9.2, 9.6, 10.5, 12.1, 12.6, 12.6, 12.6, 13.8, 13.9, 14.8, 15.5, 17.4, 18, 18.2, 18.2, 18.7, 18.8, 19.1, 19.3, 19.5, 20.2, 21.2, 21.7, 22.1, 22.7, 22.9, 23.1, 24.5, 25.6, 26, 26.6, 27.2, 27.3, 27.5, 28.3, 28.4, 28.7, 28.9, 29.9, 30.2, 30.2, 30.3, 31, 31.1, 31.2, 31.2, 31.3, 31.4, 31.6, 31.7, 31.8, 31.9, 32.5, 32.8, 32.8, 33.3, 33.5, 35.1, 35.4, 35.8, 36.7, 37.7, 37.9, 39, 39.1, 39.4, 39.4, 39.6, 40, 40.3, 41, 41.1, 41.7, 41.8, 42, 42.7, 42.7, 43.9, 44.9};

static void drawCarFrontFrame(float current_roll, uint16_t bodyColor, uint16_t lineColor, uint16_t textColor)
{
  int cx = 120;
  int cy = 120;

  float angle = current_roll * DEG2RAD;
  float s = sin(angle);
  float c = cos(angle);

  // Car body - trapezoidal shape (wider at back/bottom)
  float bodyFrontW = 35; // Front (narrow)
  float bodyBackW = 70;  // Back (wide)
  float bodyH = 50;      // Height

  // Trapezoid points (car viewed from behind)
  // Front (top) - narrow
  float x1 = -bodyFrontW / 2, y1 = -bodyH / 2;
  float x2 = bodyFrontW / 2, y2 = -bodyH / 2;
  // Back (bottom) - wide
  float x3 = bodyBackW / 2, y3 = bodyH / 2;
  float x4 = -bodyBackW / 2, y4 = bodyH / 2;

  // Draw car body (filled trapezoid)
  int rx1 = cx + x1 * c - y1 * s;
  int ry1 = cy + x1 * s + y1 * c;
  int rx2 = cx + x2 * c - y2 * s;
  int ry2 = cy + x2 * s + y2 * c;
  int rx3 = cx + x3 * c - y3 * s;
  int ry3 = cy + x3 * s + y3 * c;
  int rx4 = cx + x4 * c - y4 * s;
  int ry4 = cy + x4 * s + y4 * c;

  // Fill the trapezoid with two triangles
  gfx->fillTriangle(rx1, ry1, rx2, ry2, rx3, ry3, bodyColor);
  gfx->fillTriangle(rx1, ry1, rx3, ry3, rx4, ry4, bodyColor);

  // Rear window (small rectangle near top)
  float winW = 25;
  float winH = 15;
  float winX1 = -winW / 2, winY1 = -bodyH / 2 + 8;
  float winX2 = winW / 2, winY2 = -bodyH / 2 + 8;
  float winX3 = winW / 2, winY3 = -bodyH / 2 + 20;
  float winX4 = -winW / 2, winY4 = -bodyH / 2 + 20;

  int wrx1 = cx + winX1 * c - winY1 * s;
  int wry1 = cy + winX1 * s + winY1 * c;
  int wrx2 = cx + winX2 * c - winY2 * s;
  int wry2 = cy + winX2 * s + winY2 * c;
  int wrx3 = cx + winX3 * c - winY3 * s;
  int wry3 = cy + winX3 * s + winY3 * c;
  int wrx4 = cx + winX4 * c - winY4 * s;
  int wry4 = cy + winX4 * s + winY4 * c;

  gfx->fillTriangle(wrx1, wry1, wrx2, wry2, wrx3, wry3, BLACK);
  gfx->fillTriangle(wrx1, wry1, wrx3, wry3, wrx4, wry4, BLACK);

  // Direction indicator lines from center
  drawLineFromCenter(current_roll, 90, 3, lineColor);
  drawLineFromCenter(180 + current_roll, 90, 3, lineColor);

  // Text for the current angle
  char buffer[20];
  int strLength = snprintf(buffer, sizeof(buffer), "%.0f%c", current_roll, 248);

  // Start of the text based on its length
  int startX = 120 - (strLength * 18) / 2;

  gfx->setTextSize(3);
  gfx->setCursor(startX, 180);
  gfx->setTextColor(textColor, BLACK);
  gfx->print(buffer);
}

static void drawCarSideFrame(float current_roll, uint16_t bodyColor, uint16_t lineColor, uint16_t textColor)
{
  int cx = 120;
  int cy = 120;

  float angle = current_roll * DEG2RAD;
  float s = sin(angle);
  float c = cos(angle);

  float bodyW = 90;
  float bodyH = 30;

  float bx1 = -bodyW / 2, by1 = -bodyH / 2;
  float bx2 = bodyW / 2, by2 = -bodyH / 2;
  float bx3 = bodyW / 2, by3 = bodyH / 2;
  float bx4 = -bodyW / 2, by4 = bodyH / 2;

  int brx1 = cx + bx1 * c - by1 * s;
  int bry1 = cy + bx1 * s + by1 * c;
  int brx2 = cx + bx2 * c - by2 * s;
  int bry2 = cy + bx2 * s + by2 * c;
  int brx3 = cx + bx3 * c - by3 * s;
  int bry3 = cy + bx3 * s + by3 * c;
  int brx4 = cx + bx4 * c - by4 * s;
  int bry4 = cy + bx4 * s + by4 * c;

  gfx->fillTriangle(brx1, bry1, brx2, bry2, brx3, bry3, bodyColor);
  gfx->fillTriangle(brx1, bry1, brx3, bry3, brx4, bry4, bodyColor);

  float cabW = 40;
  float cabH = 20;
  float cabYTop = -bodyH / 2 - cabH;
  float cx1 = -cabW / 2, cy1 = cabYTop;
  float cx2 = cabW / 2, cy2 = cabYTop;
  float cx3 = cabW / 2, cy3 = -bodyH / 2;
  float cx4 = -cabW / 2, cy4 = -bodyH / 2;

  int crx1 = cx + cx1 * c - cy1 * s;
  int cry1 = cy + cx1 * s + cy1 * c;
  int crx2 = cx + cx2 * c - cy2 * s;
  int cry2 = cy + cx2 * s + cy2 * c;
  int crx3 = cx + cx3 * c - cy3 * s;
  int cry3 = cy + cx3 * s + cy3 * c;
  int crx4 = cx + cx4 * c - cy4 * s;
  int cry4 = cy + cx4 * s + cy4 * c;

  gfx->fillTriangle(crx1, cry1, crx2, cry2, crx3, cry3, bodyColor);
  gfx->fillTriangle(crx1, cry1, crx3, cry3, crx4, cry4, bodyColor);

  // Direction indicator lines from center
  drawLineFromCenter(current_roll, 90, 3, lineColor);
  drawLineFromCenter(180 + current_roll, 90, 3, lineColor);

  char buffer[20];
  int strLength = snprintf(buffer, sizeof(buffer), "%.0f%c", current_roll, 248);
  int startX = 120 - (strLength * 18) / 2;

  gfx->setTextSize(3);
  gfx->setCursor(startX, 180);
  gfx->setTextColor(textColor, BLACK);
  gfx->print(buffer);
}

float current_roll = 0.0;
float old_roll = 0.0;
// we use this variable to update the screen only every 200ms, to avoid unnecessary updates and improve performance
unsigned long lastUpdateTime = 0;

void drawCarFront()
{
  // unsigned long now = millis();
  // if (now - lastUpdateTime > 40) // update every 40ms
  // {
  //   float roll = roll = rotAngles[++rotIndex % 100]; // example angle, you can replace it with actual data
  //   old_roll = roll;
  //   lastUpdateTime = now;
  // }
  // else
  // {
  //   roll = old_roll;
  // }

  // float roll = rotAngles[++rotIndex % 100]; // example angle, you can replace it with actual data
  float roll = rotAngles[++rotIndex % 100]; // example angle, you can replace it with actual data

  current_roll = current_roll + (roll - current_roll) * smooth;

  if (fabs(current_roll - old_roll) <= 1.0)
  {
    return;
  }

  drawCarFrontFrame(old_roll, BLACK, BLACK, BLACK);

  drawCarFrontFrame(current_roll, ui_color, ui_color, WHITE);

  old_roll = current_roll;
}

float current_pitch = 0.0;
float old_pitch = 0.0;

void drawPitch()
{

  float pitch = rotAngles[++rotIndex % 100]; // example angle, you can replace it with actual data

  current_pitch = current_pitch + (pitch - current_pitch) * smooth;

  if (fabs(current_pitch - old_pitch) <= 1.0)
  {
    return;
  }

  drawCarSideFrame(old_pitch, BLACK, BLACK, BLACK);

  drawCarSideFrame(current_pitch, ui_color, ui_color, WHITE);

  old_pitch = current_pitch;
}

void drawInit()
{
  char *msg = (char *)"Errore sconosciuto";

  if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
  {
    auto i = errorMap.find(err);

    if (i != errorMap.end())
    {
      msg = i->second;
    }
    xSemaphoreGive(xUIMutex);
  }

  int length = strlen(msg);
  int startX = 120 - (length * 6) / 2;
  gfx->setTextSize(1);
  gfx->setCursor(startX, 190);
  gfx->setTextColor(WHITE);
  gfx->print(msg);
}