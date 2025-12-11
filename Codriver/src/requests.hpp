#ifndef REQUESTS_HPP
#define REQUESTS_HPP

#include <WiFi.h>

// dichiara le funzioni
void setupWifi();
int getRPM();
int getFuel();
int getBoost();
long sendOBDCommand(String, int);

#endif
