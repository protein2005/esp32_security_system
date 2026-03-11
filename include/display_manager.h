#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "models.h"

class DisplayManager {
public:
  DisplayManager();
  bool begin();
  void showSystemState(const SensorData &data, const SystemState &state, const String &msg);
  void showAlarm(const String &reason);
  void showStartup();

private:
  Adafruit_SSD1306 display;
};

#endif