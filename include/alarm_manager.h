#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <Arduino.h>

class AlarmManager {
public:
  void begin();
  void trigger();
  void stop();
};

#endif