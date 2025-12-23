  #include <Arduino.h>
  #include <Arduino_GFX_Library.h>

  // questi servono per agire sul filesystem
  #include <LittleFS.h>
  #include <FS.h>

  #include "global.hpp"

  // pin dello schermetto
  #define TFT_MOSI 11
  #define TFT_SCLK 12  
  #define TFT_DC   6
  #define TFT_CS   10
  #define TFT_RST  7
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
  void setupScreen(){
    
    gfx->begin();
    gfx->invertDisplay(true);
    gfx->fillScreen(RED);
    
    // inizializzo qui il sistema
    if (!LittleFS.begin(true)) {
      Serial.println("❌ Errore critico: Impossibile inizializzare LittleFS!");
      while(true); 
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
    while(!is_tcp_connected) {
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }

    if (xSemaphoreTake(xUIMutex, portMAX_DELAY) == pdTRUE)
    {
      // just for testing
        ui_color = loadState("color");
        ui_index = loadState("screen");
        float min = loadState("min");
        float max = loadState("max");
        if(min!=-1){
            OBDScreens[ui_index].min = min;
        }
        if(max!=-1){
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

  void changeBitmap(int index){
    int current_index = index;
    
    Serial.printf("Preso file: %s\n", OBDScreens[current_index].bitmap_file);
    // load the brtmap from LittleFS
    File file = LittleFS.open(OBDScreens[current_index].bitmap_file, "r");
    if (!file) {
      Serial.printf("ERRORE: Impossibile aprire il file: %s\n", OBDScreens[current_index].bitmap_file); 
      return;
    }

    gfx->fillScreen(BLACK);
    
    // we draw a screen only if it has a draw function
    if (OBDScreens[current_index].drawFunction) {
      for (int16_t i = 0; i < 240; i++) {
          file.readBytes((char*)lineBuffer, LINE_BUFFER_SIZE);
          gfx->draw16bitBeRGBBitmap(0, i, lineBuffer, 240, 1); 
      }
    }

    file.close();
  }

  float toAngle(float value, float minVal, float maxVal, float startAngle, float sweepAngle){
    if (maxVal - minVal == 0) return startAngle;
    if (value < minVal) value = minVal;
    if (value > maxVal) value = maxVal;

    float percent = (value - minVal) / (maxVal - minVal);
    return startAngle + percent * sweepAngle;
  }

  void drawLineFromCenter(float degrees, int length, int thickness, int color) {
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

  void drawGauge(float value){
   // Serial.println(value);
    if (value == -1) {
      value = val;
    }

    // smoothing
    target_deg = toAngle(value, current_min, current_max, 170.0, 200.0);
    current_deg = current_deg + (target_deg - current_deg) * smooth;

    int current_color;
    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        current_color = ui_color;
        xSemaphoreGive(xUIMutex);
    } else {
        current_color = ui_color; 
    }

    drawLineFromCenter(deg, LENGTH, 5, BLACK);
    drawLineFromCenter(current_deg, LENGTH, 5, current_color);

    deg = current_deg;
  }

  void drawScreen(){
    int current_index;
    bool update = false;
    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        current_index = ui_index;
        if(ui_update){
          update = true;
          ui_update = false;
        }
        xSemaphoreGive(xUIMutex);
    } else {
        current_index = ui_index;
    }

    // Serial.printf("drawing screen: %d\n", current_index);
    // Serial.printf("drawing screen: %d\n", current_index);

    DataTypes_t screen = OBDScreens[current_index];

    if(update){
      if(getError() != 0) {
        changeBitmap(0);
      } else {
        Serial.printf("Updating UI! index: %d\n", ui_index);
        changeBitmap(current_index);
        current_min = screen.min;
        current_max = screen.max;
      }
      
    }
    
    if(getError() != 0){
      OBDScreens[0].drawFunction();
      return;
    }

    if (screen.drawFunction) {
      screen.drawFunction();
    }
  }

  // here u can add new draw functions for the various screens



  void drawBoost(){
    float boostValue;
    if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        boostValue = liveData.boost;
        // important to set those values to -1 after reading
        // for easy smoothing
        liveData.boost = -1.0;
        xSemaphoreGive(xDataMutex);
    } else {
        boostValue = liveData.boost;
    }

    // Serial.printf("boost: %f, min: %f, max: %f\n", boostValue, current_min, current_max);

    // i get the length of the string
    int length = snprintf(NULL, 0, "%.*f", decimals, boostValue);
    int prevlength = snprintf(NULL, 0, "%.*f", decimals, val);

    drawGauge(boostValue);

    if( boostValue == -1.0 ){
      return;
    }

    int startX = 120 - (length * 18)/2;
    int prevStartX = 120 - (prevlength * 18)/2;
     // da vedere questo
    if (abs(boostValue - val) > 0.001) {
      //cancello
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


  void drawRPM(){
    float value;
    if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        value = liveData.rpm;
        // important to set those values to -1 after reading
        // for easy smoothing
        liveData.rpm = -1.0;
        xSemaphoreGive(xDataMutex);
    } else {
        value = liveData.rpm;
    }

    // Serial.print("rpm: ");
    // Serial.println(value);
    
    int length = snprintf(NULL, 0, "%.*f", decimals, value);
    
    drawGauge(value);
    
    int startX = 120 - (length * 18)/2;
    if( value == -1 ){return; }
     // da vedere questo
    if (abs(value - val) > 0.001) {
      //cancello
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


  void drawAcceleration(){
    float oldax = 0, olday = 0, oldaz = 0;
    float ax, ay, az;

    // get the previous values in case of change
    oldax = liveData.accelX;
    olday = liveData.accelY;
    oldaz = liveData.accelZ;

    if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
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
    else {
        ax = liveData.accelX;
        ay = liveData.accelY;
        az = liveData.accelZ;
    }

    


    //cancello
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



    //scrivo
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

  void drawInit(){
    char * msg = "Errore non riconosciuto";

    if(xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE){
      auto i = errorMap.find(err);

      if (i != errorMap.end()) {
        msg = i->second;
      }
      xSemaphoreGive(xUIMutex);
    }

    int length = snprintf(NULL, 0, msg);
    int startX = 120 - (length * 6)/2;
    gfx->setTextSize(1);
    gfx->setCursor(startX, 190);
    gfx->setTextColor(WHITE);
    gfx->print(msg);
  }