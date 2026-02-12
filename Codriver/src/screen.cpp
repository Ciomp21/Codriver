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
  int current_index = 3;
  bool update = false;

  if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    // current_index = ui_index;

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

void drawBoost()
{
  float boostValue;
  if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    boostValue = liveData.boost;
    // important to set those values to -1 after reading
    // for easy smoothing
    liveData.boost = -1.0;
    xSemaphoreGive(xDataMutex);
  }
  else
  {
    boostValue = liveData.boost;
  }

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

void drawAcceleration()
{
  float oldax = 0, olday = 0, oldaz = 0;
  float ax, ay, az;

  // get the previous values in case of change
  oldax = liveData.accelX;
  olday = liveData.accelY;
  oldaz = liveData.accelZ;

  if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    // these are the new values to read
    ax = liveData.accelX;
    ay = liveData.accelY;
    az = liveData.accelZ;

    // important to set those values to -1 after reading
    // for easy smoothing
    liveData.accelX = -1.0;
    liveData.accelY = -1.0;
    liveData.accelZ = -1.0;
    xSemaphoreGive(xDataMutex);
  }
  else
  {
    ax = liveData.accelX;
    ay = liveData.accelY;
    az = liveData.accelZ;
  }

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
float oldAngle = 0;

void drawCarFront()
{
  float angleDeg = rotAngles[++rotIndex % 100]; // example angle, you can replace it with actual data
  // float angleDeg = 20.0; // example angle, you can replace it with actual data

  if (fabs(angleDeg - oldAngle) > 0.1)
  {

    oldAngle = angleDeg;
    int cx = 120;
    int cy = 120;

    float angle = angleDeg * DEG2RAD;
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

    gfx->fillCircle(120, 120, 92, BLACK);

    // Fill the trapezoid with two triangles
    gfx->fillTriangle(rx1, ry1, rx2, ry2, rx3, ry3, ui_color);
    gfx->fillTriangle(rx1, ry1, rx3, ry3, rx4, ry4, ui_color);

    // // Draw outline
    // gfx->drawLine(rx1, ry1, rx2, ry2, WHITE);
    // gfx->drawLine(rx2, ry2, rx3, ry3, WHITE);
    // gfx->drawLine(rx3, ry3, rx4, ry4, WHITE);
    // gfx->drawLine(rx4, ry4, rx1, ry1, WHITE);

    // Rear window (small rectangle near top)
    float winW = 20;
    float winH = 12;
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
    drawLineFromCenter(angleDeg, 90, 3, ui_color);
    drawLineFromCenter(180 + angleDeg, 90, 3, ui_color);

    // Text for the current angle
    char buffer[20];
    int STRlength = snprintf(buffer, sizeof(buffer), "%.0f%c", angleDeg, 248);

    // Start of the text based on its length
    int startX = 120 - (STRlength * 18) / 2;

    gfx->setTextSize(3);
    gfx->setCursor(startX, 180);
    gfx->setTextColor(WHITE);
    gfx->print(buffer);
  }
}

void drawPitch()
{
  float angleDeg = 20.0; // example angle, you can replace it with actual data
  int cx = 120;
  int cy = 160;

  float angle = angleDeg * DEG2RAD;
  float s = sin(angle);
  float c = cos(angle);

  float w = 50;
  float h = 15;

  // Pivot slightly rearward
  float pivotX = -w / 4;
  float pivotY = 0;

  float x1 = -w / 2 - pivotX, y1 = -h / 2 - pivotY;
  float x2 = w / 2 - pivotX, y2 = -h / 2 - pivotY;
  float x3 = w / 2 - pivotX, y3 = h / 2 - pivotY;
  float x4 = -w / 2 - pivotX, y4 = h / 2 - pivotY;

  drawRotatedLine(cx, cy, x1, y1, x2, y2, s, c);
  drawRotatedLine(cx, cy, x2, y2, x3, y3, s, c);
  drawRotatedLine(cx, cy, x3, y3, x4, y4, s, c);
  drawRotatedLine(cx, cy, x4, y4, x1, y1, s, c);
}

void drawInit()
{
  char *msg = "Errore non riconosciuto";

  if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
  {
    auto i = errorMap.find(err);

    if (i != errorMap.end())
    {
      msg = i->second;
    }
    xSemaphoreGive(xUIMutex);
  }

  int length = snprintf(NULL, 0, msg);
  int startX = 120 - (length * 6) / 2;
  gfx->setTextSize(1);
  gfx->setCursor(startX, 190);
  gfx->setTextColor(WHITE);
  gfx->print(msg);
}