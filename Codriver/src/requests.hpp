#ifndef REQUESTS_HPP
#define REQUESTS_HPP

#include <WiFi.h>

// dichiara le funzioni
void setupWifi();
void checkWifiStatus();
int checkConnection();
int sendOBDCommand(const char* pid);

#endif
