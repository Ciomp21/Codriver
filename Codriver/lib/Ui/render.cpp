#include <Arduino.h>
#include "render.h"
#include "livedata.h"
#include "widgets.h"

Preset currentPreset[3]; // Presets for three displays

void InitDisplays()
{
    ChangePreset();
    // Other display initialization code can go here
}

void DisplayRefresh(const Values *data)
{
}

void ChangePreset()
{
    // Change the current preset (this is just a placeholder implementation)
    // Should actually read the displayConfig file and the presets file to load presets
    // This function should also load static assets if needed (e.g., icons, backgrounds)

    // Example: Load a simple preset for demonstration
    currentPreset[0].widgets = {
        {RPM_METER, RPM, 10, 10, 200, 20, 0.0f},
    };
    currentPreset[1].widgets = {
        {SPEED_NUMBER, SPEED, 10, 10, 100, 50, 0.0f},
    };

    currentPreset[2].widgets = {
        {BAR, ACCEL_X, 10, 10, 200, 20, 0.0f},
    };
}
