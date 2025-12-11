#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

extern int color;

enum types {
    GAUGE,
    TEMP,
    ACCEL,
    NONE
};

void setupScreen();
// chiamate questa funzione per settare la bitmap e specificare il tipo di draw
// usando l'enum sopra
void changeBitmap(enum types t, int bitmap);

// disegna automaticamente 
void drawScreen(float value, float min, float max);

#endif
