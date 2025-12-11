#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// questi servono per agire sul filesystem
#include <LittleFS.h>
#include <FS.h>

// questo serve per salvare lo stato ad ogni cambiamento
#include <Preferences.h>

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

// Definizione Assunta (Dovrebbe essere in un .hpp per codice esterno)
enum types{
  GAUGE,
  TEMP,
  ACCEL,
  NULLTYPE,
};

Preferences preferences;
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0);

int color = 0xFFFFFF;
float deg = 0;
float val = 0;
enum types currentType = NULLTYPE;

static uint16_t lineBuffer[240]; 
const char* const bitmaps[] = {
    "/boost.bin",
    "/gforce.bin"
};
const int TOTAL_BITMAPS = sizeof(bitmaps) / sizeof(bitmaps[0]);

void saveState(const char* state, int val);
int loadState(const char* state);
void changeBitmap(enum types t, int index);



// Preparea lo schermo e LittleFS
void setupScreen(){
  
  gfx->begin();
  gfx->invertDisplay(true);
  gfx->fillScreen(BLACK);
  
  // inizializzo qui il sistema
  if (!LittleFS.begin(true)) {
    Serial.println("❌ Errore critico: Impossibile inizializzare LittleFS!");
    while(true); 
  }
  Serial.println("✅ LittleFS avviato.");

  color = loadState("color");
  int index = loadState("screen");
  enum types t = (enum types) loadState("type");

  changeBitmap(t, index);
  
}

//utilizziamo bitmap presalvate all'interno dell'esp
void changeBitmap(enum types t, int index){
  if (index < 0 || index >= TOTAL_BITMAPS) {
      Serial.printf("ERRORE: Indice bitmap non valido: %d\n", index);
      return;
  }

  // apro un file per leggere la bitmap e poi lo chiudo
  File file = LittleFS.open(bitmaps[index], "r");
  if (!file) {
    Serial.printf("ERRORE: Impossibile aprire il file: %s\n", bitmaps[index]); 
    return;
  }

  currentType = t;
  gfx->fillScreen(BLACK); // Puliamo lo schermo
  
  // alla fine la soluzione migliore e' scrivere riga per riga
  if (t != NULLTYPE) {
    for (int16_t i = 0; i < 240; i++) {
        file.readBytes((char*)lineBuffer, LINE_BUFFER_SIZE);
        gfx->draw16bitBeRGBBitmap(0, i, lineBuffer, 240, 1); 
    }
  }

  file.close();

  // salvo gli stati

  saveState("color", color);
  saveState("screen", index);
  saveState("type", currentType);
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
// domanda lecita: salviamo tutte le bitmap sull'esp o gliele passiamo
// come argomento? dobbiamo testare

void drawGauge(float value, float min, float max){
  // calcolo angolo

  float lastdeg = deg;
  deg = toAngle(value, min, max, 130.0, 200.0);

  // copro la scritta di prima di nero e poi la ridisegno
  drawLineFromCenter(lastdeg, LENGTH, 5, BLACK);
  drawLineFromCenter(deg, LENGTH, 5, color);

  // Se il valore non è cambiato, non ridisegno il testo (anti-flicker)
  if (abs(value - val) < 0.001) return;

  //cancello
  gfx->setTextSize(3);
  gfx->setCursor(90, 150);
  gfx->setTextColor(BLACK);
  gfx->print(val, 2);

  val = value;

  //scrivo
  gfx->setTextSize(3);
  gfx->setCursor(90, 150);
  gfx->setTextColor(WHITE);
  gfx->print(value, 2);
}

void drawScreen(float value, float min, float max){
  // per ogni metodo di stampa possiamo inserire un nuovo switch
  switch (currentType){
    case GAUGE:
      drawGauge(value, min, max);
    break;
    default:
      //cancello
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

void saveState(const char* state, int val) {
    preferences.begin("infotainment", false); // Apre lo spazio di archiviazione
    preferences.putInt(state, val);  // Salva l'indice
    preferences.end();
}

int loadState(const char* state) {
    preferences.begin("infotainment", true); // Apre in sola lettura
    int ret = preferences.getInt(state, 0); // 0 è il valore di default
    preferences.end();
    return ret;
}