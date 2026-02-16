#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <Arduino_GFX_Library.h>
#include "global.hpp"

#define CENTER_X 120
#define CENTER_Y 120
#define RADIUS 100
#define MAX_G 2.0

#define GFORCE_INTERVAL 90

// pin dello schermetto
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS 10
#define TFT_DC 6
#define TFT_RST 7
#define LENGTH 70

enum Quadrant
{
    NONE = -1,
    Q1,
    Q2,
    Q3,
    Q4,

};
enum Force
{
    ZEROF = -1,
    LOWF,
    MEDIUMF,
    HIGHF
};

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
