#include <Arduino.h>
#include "requests.hpp"
#include "screen.hpp"
#include "bleconnection.hpp"

// si devono usare per le bitmap
#include <FS.h> 
#include <LittleFS.h>

void saveState(int screen, int color){

}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // inizializziamo tutte le librerie del caso
    //setupWifi(); 
    // setupBLE();
    setupScreen();

    //settiamo la libreria giusta
    changeBitmap(GAUGE, 0);
    delay(2000);
}

void loop() {
    delay(100); // limitatore (si può togliere)

    // int rpm = getRPM(); 
    // float boost = getBoost();

    drawScreen(1.0, 0.0, 5400.0);

    
    // ________________ print vari per test ________________
    // if (rpm >= 0) {
    //     Serial.print("RPM: ");
    //     Serial.println(rpm);
    // } else {
    //     Serial.println("⚠️ Nessun dato RPM ricevuto");
    // }

    // if (boost >= 0) {
    //     Serial.print("BOOST: ");
    //     Serial.print((boost/100) - 1);
    //     Serial.println(" bar");
    // } else {
    //     Serial.println("⚠️ Nessun dato BOOST ricevuto");
    // }
    // ____________ fine print vari ________________



    Serial.print("temperatura: ");
    Serial.println( temperatureRead() );

}
