#include <Arduino.h>

#define LED_PIN 6  // LED da far lampeggiare

void setup() {
  Serial.begin(115200);
  while(!Serial); // aspetta che la seriale sia pronta
  Serial.println("Setup iniziato");

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(1000);

  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(1000);
}
