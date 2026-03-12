#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <Arduino.h>

class AlarmManager {
public:
  void begin();
  void update();
  void setVisualAlert(bool enabled);
  void startSound();
  void stopSound();
  void stopAll();

private:
  bool soundEnabled = false;
  unsigned long lastToneChange = 0;
  size_t toneStep = 0;
};

#endif
