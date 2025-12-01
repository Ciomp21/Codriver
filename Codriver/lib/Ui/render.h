
#pragma once
#include "Arduino.h"
#include "livedata.h"
#include "widgets.h"
#include <vector>

void InitDisplays();
void DisplayRefresh(LiveData* data);
void ChangePreset();

struct Preset {
    // Define preset structure here
    std::vector<Widget> widgets; // Example: array of widgets
};
