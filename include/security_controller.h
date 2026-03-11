#ifndef SECURITY_CONTROLLER_H
#define SECURITY_CONTROLLER_H

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

  SensorData sensorData;
  SystemState systemState;

  unsigned long lastSensorRead = 0;
  unsigned long lastHeartbeat = 0;

  void processCommands(const String &cmd);
  void handleAlarm(const String &reason);
  static SecurityController *instance;
  static void commandProxy(const String &cmd);
};

#endif