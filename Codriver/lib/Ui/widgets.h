
#pragma once

enum WidgetType
{
    RPM_METER,
    SPEED_NUMBER,
    BAR,
    ICON,

    // Add other widget types as needed
};

enum WidgetMetric
{
    RPM,
    SPEED,
    ACCEL_X,
    
    // Add other fields as needed
};

//* This header defines simple UI widget types and a compact Widget descriptor used to place and configure widgets on a display.
//* WidgetType: enumerates rendering/visual kinds (RPM_METER, SPEED_NUMBER, BAR, …).
//* WidgetField: enumerates data sources a widget can show (FIELD_RPM, FIELD_SPEED, FIELD_ACCEL_X, …).
//* Widget struct:
//*    - type: which WidgetType to render.
//*    - x, y: top-left position (pixels).
//*    - w, h: width and height (pixels).
//*    - field: which data field the widget displays.
//* Usage: create a Widget instance, set type to choose the renderer, set x/y/w/h for layout, and set field to bind the widget to a data source (e.g., rpm, speed).
struct Widget
{
    WidgetType type;   // Use enum for widget type
    WidgetMetric metric; // which data to show ("rpm", "speed", "acc_x")
    int x, y, w, h;    // position + size
    float rotation; // for widgets that need rotation (e.g., needle)
    

};
