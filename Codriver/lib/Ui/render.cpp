#include <Arduino.h>
#include "render.h"
#include "livedata.h"
#include "widgets.h"
#include <tft_eSPI.h> // Graphics and font library for ST7735 driver chip

// Initialize the display
TFT_eSPI tft1 = TFT_eSPI(); // Invoke library
TFT_eSPI tft2 = TFT_eSPI(); // Invoke library
TFT_eSPI tft3 = TFT_eSPI(); // Invoke library

Preset currentPreset[3]; // Presets for three displays

void InitDisplays()
{

    tft1.init();
    tft2.init();
    tft3.init();

    ChangePreset();

    // Other display initialization code can go here
}

void DisplayRefresh(const LiveData* data)
{
    TFT_eSprite sprites[3] = {TFT_eSprite(&tft1), TFT_eSprite(&tft2), TFT_eSprite(&tft3)};

    // Example: Clear the screens with different colors
    sprites[0].createSprite(240, 240);
    sprites[1].createSprite(240, 240);
    sprites[2].createSprite(240, 240);

    sprites[0].fillSprite(TFT_BLACK);
    sprites[1].fillSprite(TFT_BLACK);
    sprites[2].fillSprite(TFT_BLACK);

    // Render widgets from the current preset
    for (int i = 0; i < 3; i++)
    {
        for (auto &widget : currentPreset[i].widgets)
        {
            // Simple rendering logic based on widget type
        }
    }

    // Push sprites to the displays
    sprites[0].pushSprite(0, 0);
    sprites[1].pushSprite(0, 0);
    sprites[2].pushSprite(0, 0);

    // Delete sprites to free memory
    sprites[0].deleteSprite();
    sprites[1].deleteSprite();
    sprites[2].deleteSprite();
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
