#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "models.h"

class NetworkManager {
public:
  NetworkManager();

  void begin();
  void update(SystemState &state);

  bool publishTelemetry(const SensorData &data, SystemState &state);
  bool publishAlarm(const String &reason, SystemState &state);
  bool publishHeartbeat(SystemState &state);
  bool publishStatus(const String &status, SystemState &state);
  bool publishEvent(const String &eventName, const String &source, const String &details, SystemState &state);

  void setCommandHandler(void (*handler)(const String&));
  PubSubClient& getClient();

private:
  WiFiClient wifiClient;
  PubSubClient mqttClient;
  unsigned long lastReconnectAttempt;
  QueuedEvent offlineQueue[OFFLINE_QUEUE_CAPACITY];
  size_t queueHead = 0;
  size_t queueSize = 0;

  String telemetryTopic;
  String alarmTopic;
  String statusTopic;
  String heartbeatTopic;
  String eventTopic;
  String cmdTopic;

  void buildTopics();
  void connectWiFi(SystemState &state);
  bool connectMQTT(SystemState &state);
  bool publishOrQueue(const String &topic, const String &payload, SystemState &state);
  void enqueueEvent(const String &topic, const String &payload, SystemState &state);
  void flushOfflineQueue(SystemState &state);
  void populateBasePayload(JsonDocument &doc) const;
  void populateThresholdPayload(JsonDocument &doc, const SystemState &state) const;

  static void mqttCallback(char *topic, byte *payload, unsigned int length);
  static void (*externalCommandHandler)(const String&);
};

#endif
