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
  float current_deg = 0;
  float smooth = 0.5;
  float val = 0;

  static uint16_t lineBuffer[240]; 
  void changeBitmap();

  // Preparea lo schermo e LittleFS
  void setupScreen(){
    
    gfx->begin();
    gfx->invertDisplay(true);
    gfx->fillScreen(RED);
    
    // inizializzo qui il sistema
    if (!LittleFS.begin(true)) {
      Serial.println("❌ Errore critico: Impossibile inizializzare LittleFS!");
      while(true); 
    }
    Serial.println("✅ LittleFS avviato.");

    changeBitmap();
  }

  //utilizziamo bitmap presalvate all'interno dell'esp
  void changeBitmap(){
    //anche qui ho bisogno di controllare i semafori
    // se non lo ricevo entro 100 ms allora uso quelli vecchi

    int current_index;

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        current_index = ui_index;

        xSemaphoreGive(xUIMutex);
    } else {
        current_index = ui_index;
    }

    // apro un file per leggere la bitmap e poi lo chiudo
    File file = LittleFS.open(OBDScreens[current_index].bitmap_file, "r");
    if (!file) {
      Serial.printf("ERRORE: Impossibile aprire il file: %s\n", OBDScreens[current_index].bitmap_file); 
      return;
    }

    gfx->fillScreen(BLACK); // Puliamo lo schermo
    
    if (OBDScreens[current_index].type != NULLTYPE) {
      for (int16_t i = 0; i < 240; i++) {
          file.readBytes((char*)lineBuffer, LINE_BUFFER_SIZE);
          gfx->draw16bitBeRGBBitmap(0, i, lineBuffer, 240, 1); 
      }
    }

    file.close();
  }

  float toAngle(float value, float minVal, float maxVal, float startAngle, float sweepAngle){
    // check divisione per zero e massimo-minimo
    if (maxVal - minVal == 0) return startAngle;
    if (value < minVal) value = minVal;
    if (value > maxVal) value = maxVal;

    // calcolo la percentuale sulla scala e la moltiplico per l'angolo massimo
    float percent = (value - minVal) / (maxVal - minVal);
    return startAngle + percent * sweepAngle;
  }

  // funzione per disegnare una rettangolo dal centro, una lancetta in pratica
  // per gauge lunghezza fissa
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



  // _____________ FUNZIONI PER MODALITA' ________________
  // da qui in poi metto come possono essere disegnati i vari display

  void drawGauge(float value, float min, float max, int decimals){
    // calcolo angolo

    Serial.println(value);
    if (value == -1) {
      value = val;
    }

    // tentativo di attuare lo smoothing abbastanza riuscito

    target_deg = toAngle(value, min, max, 130.0, 200.0);
    current_deg = current_deg + (target_deg - current_deg) * smooth;

    //anche qui prendo il controllo delle variabili globali, oppure uso quelle di prima
    // se non riesco
    int current_color;
    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        current_color = ui_color;
        xSemaphoreGive(xUIMutex);
    } else {
        current_color = ui_color; 
    }

    // copro la scritta di prima di nero e poi la ridisegno
    drawLineFromCenter(deg, LENGTH, 5, BLACK);
    drawLineFromCenter(current_deg, LENGTH, 5, current_color);

    deg = current_deg;
    // Se il valore non è cambiato, non ridisegno il testo (anti-flicker)
    
    if (abs(value - val) > 0.001) {
      //cancello
      gfx->setTextSize(3);
      gfx->setCursor(110, 150);
      gfx->setTextColor(BLACK);
      gfx->print(val, decimals);
    }
    

    val = value;

    //scrivo
    gfx->setTextSize(3);
    gfx->setCursor(110, 150);
    gfx->setTextColor(WHITE);
    gfx->print(value, decimals);
  }

  void drawAcceleration(float value){

    float oldax = roundf(((int)(val / 100000) % 10) * 10) / 10.0f;
    float olday = roundf(((int)(val / 1000) % 100) * 10) / 10.0f;
    float oldaz = roundf(((int)(val / 10) % 10) * 10) / 10.0f;

    // estraggo i valori
    float ax = roundf(((int)(value / 100000) % 10) * 10) / 10.0f;
    float ay = roundf(((int)(value / 1000) % 100) * 10) / 10.0f;
    float az = roundf(((int)(value / 10) % 10) * 10) / 10.0f;

    if(value == -1) return;

    // Serial.print(value);
    // Serial.println(" totale");
    // Serial.print("ax: ");
    // Serial.println(ax);
    // Serial.print("ay: ");
    // Serial.println(ay);
    // Serial.print("az: ");
    // Serial.println(az);

    if (abs(value - val) > 0.001) {
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
    }

    val = value;

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

  void drawScreen(float value){

    int current_index;
    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        current_index = ui_index;
        xSemaphoreGive(xUIMutex);
    } else {
        current_index = ui_index;
    }

    DataTypes_t screen = OBDScreens[current_index];
    switch (screen.type){
      case GAUGE:
        drawGauge(value, screen.min, screen.max, screen.decimals);
      break;

      case GRAPH:
        //altre definizioni
        drawAcceleration(value);
      break;

      //altre definizioni
      default:
        gfx->setTextSize(3);
        gfx->setCursor(120, 120);
        gfx->setTextColor(BLACK);
        gfx->print(val, 2);

        val = value;

        //scrivo
        gfx->setTextSize(3);
        gfx->setCursor(120, 120);
        gfx->setTextColor(WHITE);
        gfx->print(value, 2);

      break;
    }



  }