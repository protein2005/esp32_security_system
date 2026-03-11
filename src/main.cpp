#include <Arduino.h>
#include "security_controller.h"

SecurityController controller;

void setup() {
  controller.begin();
}

void loop() {
  controller.update();
  delay(10);
}