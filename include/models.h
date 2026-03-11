#ifndef MODELS_H
#define MODELS_H

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
};

#endif