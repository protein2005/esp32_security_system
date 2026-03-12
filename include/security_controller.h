#ifndef SECURITY_CONTROLLER_H
#define SECURITY_CONTROLLER_H

#include <Preferences.h>
#include "models.h"
#include "sensors.h"
#include "display_manager.h"
#include "alarm_manager.h"
#include "network_manager.h"

class SecurityController {
public:
  void begin();
  void update();

private:
  Sensors sensors;
  DisplayManager display;
  AlarmManager alarm;
  NetworkManager network;
  Preferences preferences;

  SensorData sensorData;
  SystemState systemState;

  unsigned long lastSensorRead = 0;
  unsigned long lastHeartbeat = 0;

  void processCommands(const String &cmd);
  void handleAlarm(const String &reason);
  void setArmedState(bool armed);
  bool updateThresholds(float tempMin, float tempMax, float humidityMin, float humidityMax);
  void loadPersistentState();
  void saveArmedState();
  void saveThresholdState();
  bool isTemperatureOutOfRange() const;
  bool isHumidityOutOfRange() const;
  static SecurityController *instance;
  static void commandProxy(const String &cmd);
};

#endif
