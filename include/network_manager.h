#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "models.h"

class NetworkManager {
public:
  NetworkManager();

  void begin();
  void update(SystemState &state);

  bool publishTelemetry(const SensorData &data, const SystemState &state);
  bool publishAlarm(const String &reason, const SystemState &state);
  bool publishHeartbeat(const SystemState &state);
  bool publishStatus(const String &status, const SystemState &state);

  void setCommandHandler(void (*handler)(const String&));
  PubSubClient& getClient();

private:
  WiFiClient wifiClient;
  PubSubClient mqttClient;
  unsigned long lastReconnectAttempt;

  String telemetryTopic;
  String alarmTopic;
  String statusTopic;
  String heartbeatTopic;
  String cmdTopic;

  void buildTopics();
  void connectWiFi(SystemState &state);
  bool connectMQTT(SystemState &state);

  static void mqttCallback(char *topic, byte *payload, unsigned int length);
  static void (*externalCommandHandler)(const String&);
};

#endif