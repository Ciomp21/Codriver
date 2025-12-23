#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "global.hpp"

void setupScreen();
// chiamate questa funzione per settare la bitmap e specificare il tipo di draw
// usando l'enum sopra
void changeBitmap(int index);
void drawRPM();
void drawInit();
void drawBoost();
void drawAcceleration();

// disegna automaticamente 
void drawScreen(float value);

#endif
