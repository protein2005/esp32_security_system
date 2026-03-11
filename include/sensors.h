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

private:
  DHT dht;
  int lastBtnState;
};

#endif