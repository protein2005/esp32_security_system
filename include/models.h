#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>

struct SensorData {
  float temperature = 0.0f;
  float humidity = 0.0f;
  bool motionDetected = false;
  bool doorOpen = false;
  bool dhtOk = false;
};

struct SystemState {
  bool isArmed = false;
  bool offlineMode = false;
  bool mqttConnected = false;
  bool wifiConnected = false;
  bool sensorFailure = false;
  size_t queuedEvents = 0;
};

struct QueuedEvent {
  String topic;
  String payload;
};

#endif
