#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// questi servono per agire sul filesystem
#include <LittleFS.h>
#include <FS.h>
#include <screen.hpp>
#include <global.hpp>

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
// Also displays the loading screen until the wifi is connected, otherwise we might have problems with the preferences library
void setupScreen()
{

  gfx->begin();
  gfx->invertDisplay(true);
  gfx->fillScreen(RED);

  // inizializzo qui il sistema
  if (!LittleFS.begin(true))
  {
    Serial.println("Errore critico: Impossibile inizializzare LittleFS!");
    setError(1);

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
  while (!is_tcp_connected)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  if (xSemaphoreTake(xUIMutex, portMAX_DELAY) == pdTRUE)
  {
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

// This function is called to update the screen, it checks if there is a new bitmap to load and then calls the draw function of the current screen
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

// Used to to change the bitmap being displayed by taking the bitmap file from the LittleFS and drawing it on the screen, it also sets the current min and max values for the gauges
void changeBitmap(int index)
{

  int current_index = index;

  Serial.printf("Preso file: %s\n", OBDScreens[current_index].bitmap_file);
  // load the brtmap from LittleFS
  File file = LittleFS.open(OBDScreens[current_index].bitmap_file, "r");
  if (!file)
  {
    setError(3);
    Serial.printf("ERRORE: Impossibile aprire il file: %s\n", OBDScreens[current_index].bitmap_file);
    return;
  }
  resolveError(3);

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

// ==================================== AUXILIARY FUNCTIONS FOR THE SCREENS ===============================

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

void drawBattery()
{
  float batteryLevel;
  if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    batteryLevel = liveData.batteryVoltage;
    xSemaphoreGive(xDataMutex);
  }
  else
  {
    batteryLevel = liveData.batteryVoltage;
  }

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

  target_deg = toAngle(batteryLevel, current_min, current_max, 170.0, 200.0);
  current_deg = current_deg + (target_deg - current_deg) * smooth;

  int length = snprintf(NULL, 0, "Battery: %.1f%%", batteryLevel);
  int startX = 120 - (length * 3) / 2;

  gfx->setTextSize(3);
  gfx->setCursor(startX, 190);
  gfx->setTextColor(BLACK);
  gfx->print(val, 1);

  gfx->setTextSize(3);
  gfx->setCursor(startX, 190);
  gfx->setTextColor(WHITE);
  gfx->print(batteryLevel, 1);

  drawLineFromCenter(deg, LENGTH, 5, BLACK);
  drawLineFromCenter(current_deg, LENGTH, 5, current_color);

  deg = current_deg;
  val = batteryLevel;
}

void drawRotatedLine(int cx, int cy, float x1, float y1, float x2, float y2, float s, float c)
{
  int rx1 = cx + x1 * c - y1 * s;
  int ry1 = cy + x1 * s + y1 * c;

  int rx2 = cx + x2 * c - y2 * s;
  int ry2 = cy + x2 * s + y2 * c;

  gfx->drawLine(rx1, ry1, rx2, ry2, WHITE);
}

void drawCarFrontFrame(float current_roll, uint16_t bodyColor, uint16_t lineColor, uint16_t textColor)
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

void drawCarSideFrame(float current_roll, uint16_t bodyColor, uint16_t lineColor, uint16_t textColor)
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

void drawGforceDot(float accX, float accY, uint16_t color)
{

  int x = CENTER_X + (accX / MAX_G) * RADIUS;
  int y = CENTER_Y - (accY / MAX_G) * RADIUS;

  // Draw new dot
  gfx->fillCircle(x, y, 5, color);
}

void drawCross(uint16_t color)
{
  gfx->fillTriangle(CENTER_X - 86, CENTER_Y - 84, CENTER_X - 84, CENTER_Y - 86, CENTER_X + 86, CENTER_Y + 84, color);
  gfx->fillTriangle(CENTER_X + 86, CENTER_Y + 84, CENTER_X + 84, CENTER_Y + 86, CENTER_X - 84, CENTER_Y - 86, color);
  gfx->fillTriangle(CENTER_X - 86, CENTER_Y + 84, CENTER_X - 84, CENTER_Y + 86, CENTER_X + 86, CENTER_Y - 84, color);
  gfx->fillTriangle(CENTER_X + 86, CENTER_Y - 84, CENTER_X + 84, CENTER_Y - 86, CENTER_X - 84, CENTER_Y + 86, color);

  gfx->fillCircle(CENTER_X, CENTER_Y, 70 / 2, DARKGREY);
}

void drawArcs(Force force, Quadrant quadrant, bool color)
{
  int START_ANGLE;

  switch (quadrant)
  {
  case Q1:
    START_ANGLE = 45;
    break;
  case Q2:
    START_ANGLE = 135;
    break;
  case Q3:
    START_ANGLE = 225;
    break;
  case Q4:
    START_ANGLE = 315;
    break;
  default:
    START_ANGLE = 0;
    break;
  }

  switch (force)
  {
  case LOWF:
    gfx->fillArc(CENTER_X, CENTER_Y, 122 / 2, 72 / 2, START_ANGLE, START_ANGLE + GFORCE_INTERVAL, color ? GREEN : BLACK);

    break;
  case MEDIUMF:
    gfx->fillArc(CENTER_X, CENTER_Y, 122 / 2, 72 / 2, START_ANGLE, START_ANGLE + GFORCE_INTERVAL, color ? GREEN : BLACK);
    gfx->fillArc(CENTER_X, CENTER_Y, 184 / 2, 126 / 2, START_ANGLE, START_ANGLE + GFORCE_INTERVAL, color ? YELLOW : BLACK);
    break;
  case HIGHF:
    gfx->fillArc(CENTER_X, CENTER_Y, 122 / 2, 72 / 2, START_ANGLE, START_ANGLE + GFORCE_INTERVAL, color ? GREEN : BLACK);
    gfx->fillArc(CENTER_X, CENTER_Y, 184 / 2, 126 / 2, START_ANGLE, START_ANGLE + GFORCE_INTERVAL, color ? YELLOW : BLACK);
    gfx->fillArc(CENTER_X, CENTER_Y, 240 / 2, 190 / 2, START_ANGLE, START_ANGLE + GFORCE_INTERVAL, color ? RED : BLACK);
    break;

  default:
    break;
  }
}

// ==================================== DRAWING FUNCTIONS FOR THE SCREENS ===============================

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

  gfx->setTextSize(3);
  gfx->setCursor(startX, 150);
  gfx->setTextColor(WHITE);
  gfx->print(boostValue, 2);

  val = boostValue;
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
  drawGauge(value);

  int length = snprintf(NULL, 0, "%.*f", decimals, value);
  int startX = 120 - (length * 18) / 2;

  if (value == -1)
  {
    return;
  }

  if (abs(value - val) > 0.001)
  {
    gfx->fillRect(startX, 155, length * 18, 18, BLACK);
  }

  val = value;

  gfx->setTextSize(3);
  gfx->setCursor(startX, 150);
  gfx->setTextColor(WHITE);
  gfx->print(value, decimals);
}

float oldax = 0, olday = 0, oldaz = 0;
float old_magnitude = 0.0;
Quadrant old_quadrant = (Quadrant)-1; // invalid quadrant to force initial draw
Force old_force = (Force)-1;          // invalid force to force initial draw

void drawMagnitude(float magnitude, int color)
{
  char buffer[20];
  int length = snprintf(buffer, sizeof(buffer), "%.1f", magnitude);
  int startX = 123 - ((length * 18) / 2);

  gfx->setTextSize(3);
  gfx->setCursor(startX, 110);
  gfx->setTextColor(color, BLACK);
  gfx->print(buffer);
}

void drawAcceleration()
{
  float accX, accY, accZ;

  if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    // these are the new values to read
    accX = liveData.accelX;
    accY = liveData.accelY;
    accZ = liveData.accelZ;

    xSemaphoreGive(xDataMutex);
  }
  else
  {
    accX = liveData.accelX;
    accY = liveData.accelY;
    accZ = liveData.accelZ;
  }

  Quadrant quadrant;
  Force currentForce;

  if (accX >= 0.1 && accY >= 0.1)
  {
    quadrant = Q1;
  }
  else if (accX < -0.1 && accY >= 0.1)
  {
    quadrant = Q2;
  }
  else if (accX < -0.1 && accY < -0.1)
  {
    quadrant = Q3;
  }
  else if (accX >= 0.1 && accY < -0.1)
  {
    quadrant = Q4;
  }
  else
  {
    // if we are close to the center we consider it as no acceleration
    quadrant = NONE; // default to NONE when near the center
  }

  float magnitude = sqrt(accX * accX + accY * accY);

  drawMagnitude(old_magnitude, BLACK);
  drawMagnitude(magnitude, ui_color);

  old_magnitude = magnitude;

  if (magnitude < 0.5)
  {
    currentForce = LOWF;
  }
  else if (magnitude < 1.0)
  {
    currentForce = MEDIUMF;
  }
  else if (magnitude >= 1.0)
  {
    currentForce = HIGHF;
  }
  else
  {
    currentForce = ZEROF; // default to ZEROF when near the center
  }

  if (quadrant != old_quadrant && currentForce != old_force)
  {
    Serial.printf("Quadrant changed from  %d to %d\n", old_quadrant, quadrant);

    switch (quadrant)
    {
    case Q1:
      drawArcs(old_force, old_quadrant, false);
      drawArcs(currentForce, quadrant, true);
      // ONLY AT DRAW OF THE ARC, to avoid redrawing it every time
      drawCross(BLACK);
      drawCross(WHITE);

      break;

    case Q2:
      drawArcs(old_force, old_quadrant, false);
      drawArcs(currentForce, quadrant, true);
      // ONLY AT DRAW OF THE ARC, to avoid redrawing it every time
      drawCross(BLACK);
      drawCross(WHITE);

      break;
    case Q3:
      drawArcs(old_force, old_quadrant, false);
      drawArcs(currentForce, quadrant, true);

      // ONLY AT DRAW OF THE ARC, to avoid redrawing it every time
      drawCross(BLACK);
      drawCross(WHITE);

      break;
    case Q4:
      drawArcs(old_force, old_quadrant, false);
      drawArcs(currentForce, quadrant, true);

      // ONLY AT DRAW OF THE ARC, to avoid redrawing it every time
      drawCross(BLACK);
      drawCross(WHITE);

      break;

    default:
      break;
    }

    old_quadrant = quadrant;
    old_force = currentForce;
  }

  oldax = accX;
  olday = accY;
  oldaz = accZ;
}

float current_roll = 0.0;
float old_roll = 0.0;

void drawRoll()
{
  float roll;
  if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    // these are the new values to read
    roll = liveData.roll;
    xSemaphoreGive(xDataMutex);
  }
  else
  {
    roll = liveData.roll;
  }

  current_roll = current_roll + (roll - current_roll) * smooth;

  if (fabs(current_roll - old_roll) <= 1.0)
  {
    return;
  }

  drawCarFrontFrame(-old_roll, BLACK, BLACK, BLACK);

  drawCarFrontFrame(-current_roll, ui_color, ui_color, WHITE);

  old_roll = current_roll;
}

float current_pitch = 0.0;
float old_pitch = 0.0;

void drawPitch()
{

  float pitch;

  if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    // these are the new values to read
    pitch = liveData.pitch;
    xSemaphoreGive(xDataMutex);
  }
  else
  {
    pitch = liveData.pitch;
  }

  current_pitch = current_pitch + (pitch - current_pitch) * smooth;

  if (fabs(current_pitch - old_pitch) <= 1.0)
  {
    return;
  }

  drawCarSideFrame(old_pitch, BLACK, BLACK, BLACK);

  drawCarSideFrame(current_pitch, ui_color, ui_color, WHITE);

  old_pitch = current_pitch;
}

float prev_t1 = -1.0;
float prev_t2 = -1.0;

void printTemperature(float temp, int strX, int strY, uint16_t color)
{
  char tempStr[20];
  int length = snprintf(tempStr, sizeof(tempStr), "%.1fC%c", temp, 248); // 248 is the degree symbol in extended ASCII
  int startX = strX - (length * 6) / 2;

  gfx->setTextSize(3);
  gfx->setCursor(startX, strY);
  gfx->setTextColor(color);
  gfx->print(tempStr);
}

void printHumidity(float humidity, int strX, int strY, uint16_t color)
{
  char humStr[20];
  int length = snprintf(humStr, sizeof(humStr), "%.1f%%", humidity);
  int startX = strX - (length * 6) / 2;

  gfx->setTextSize(3);
  gfx->setCursor(startX, strY);
  gfx->setTextColor(color);
  gfx->print(humStr);
}

void drawTemperature()
{
  float oil_temperature = 0.0;
  float coolant_temperature = 0.0;

  if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    coolant_temperature = liveData.coolantTemp;
    xSemaphoreGive(xDataMutex);
  }
  else
  {
    coolant_temperature = liveData.coolantTemp;
  }

  printTemperature(prev_t2, 130, 115, BLACK);
  printTemperature(coolant_temperature, 130, 115, ui_color);
  prev_t2 = coolant_temperature;
}

float prev_h = 0.0;

void drawAirTemperature()
{
  float humidity = 0.0;
  float air_temperature = 0.0;

  if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
  {
    air_temperature = liveData.InternalTemp;
    humidity = liveData.humidity;
    xSemaphoreGive(xDataMutex);
  }
  else
  {
    humidity = liveData.humidity;
    air_temperature = liveData.InternalTemp;
  }

  printTemperature(prev_t1, 130, 95, BLACK);
  printTemperature(air_temperature, 130, 95, ui_color);
  prev_t1 = air_temperature;

  printHumidity(prev_h, 130, 170, BLACK);
  printHumidity(humidity, 130, 170, ui_color);
  prev_h = humidity;
}

// Used to draw the error screen, it checks the error code and displays the appropriate message
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
  gfx->setCursor(startX, 120);
  gfx->setTextColor(WHITE, BLACK);
  gfx->print(msg);
}