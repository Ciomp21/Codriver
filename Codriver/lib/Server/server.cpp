#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include "render.h"

void startWebServer()
{
  // Initialize and start the web server
  AsyncWebServer server(80);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hello, this is Codriver Webserver!"); });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", "{\"status\":\"OK\",\"message\":\"Codriver is running smoothly.\"}"); });

  server.on("/presets", HTTP_POST, [](AsyncWebServerRequest *request)
            { 
                // Handle preset update (this is just a placeholder)
                

                ChangePreset();
                
                // Respond with success message
                request->send(200, "application/json", "{\"status\":\"Preset updated successfully.\"}"); });

  server.begin();
}