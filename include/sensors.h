#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <DHT.h>
#include "models.h"

class Sensors {
public:
  Sensors();
  void begin();
  SensorData readData();
  bool isArmButtonPressed();
  bool isResetAlarmButtonPressed();

private:
  DHT dht;
  int lastArmBtnState;
  int lastResetBtnState;
  bool isButtonPressed(uint8_t pin, int &lastState);
};

#endif
