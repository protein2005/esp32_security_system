#include "network_manager.h"
#include "config.h"

void (*NetworkManager::externalCommandHandler)(const String&) = nullptr;

NetworkManager::NetworkManager()
  : mqttClient(wifiClient), lastReconnectAttempt(0) {}

void NetworkManager::buildTopics() {
  String base = "security/rooms/" + String(ROOM_ID);

  telemetryTopic = base + "/telemetry";
  alarmTopic = base + "/alarm";
  statusTopic = base + "/status";
  heartbeatTopic = base + "/heartbeat";
  cmdTopic = base + "/cmd";
}

void NetworkManager::begin() {
  WiFi.mode(WIFI_STA);
  buildTopics();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

void NetworkManager::connectWiFi(SystemState &state) {
  if (WiFi.status() == WL_CONNECTED) {
    state.wifiConnected = true;
    return;
  }

  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());

  state.wifiConnected = true;
}

bool NetworkManager::connectMQTT(SystemState &state) {
  Serial.println("Connecting to MQTT...");

  bool ok = mqttClient.connect(DEVICE_ID);
  if (ok) {
    Serial.println("MQTT connected");
    mqttClient.subscribe(cmdTopic.c_str());

    state.mqttConnected = true;
    state.offlineMode = false;

    publishStatus("ONLINE", state);
    return true;
  }

  Serial.print("MQTT failed, rc=");
  Serial.println(mqttClient.state());

  state.mqttConnected = false;
  state.offlineMode = true;
  return false;
}

void NetworkManager::update(SystemState &state) {
  if (WiFi.status() != WL_CONNECTED) {
    state.wifiConnected = false;
    connectWiFi(state);
  }

  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= MQTT_RECONNECT_INTERVAL) {
      lastReconnectAttempt = now;
      connectMQTT(state);
    }
  } else {
    state.mqttConnected = true;
  }

  mqttClient.loop();
}

bool NetworkManager::publishTelemetry(const SensorData &data, const SystemState &state) {
  if (!mqttClient.connected()) {
    return false;
  }

  char payload[320];
  snprintf(
    payload,
    sizeof(payload),
    "{\"deviceId\":\"%s\",\"roomId\":\"%s\",\"token\":\"%s\",\"temp\":%.1f,\"hum\":%.1f,\"motion\":%s,\"door\":%s,\"armed\":%s,\"offline\":%s,\"sensorFailure\":%s}",
    DEVICE_ID,
    ROOM_ID,
    DEVICE_TOKEN,
    data.temperature,
    data.humidity,
    data.motionDetected ? "true" : "false",
    data.doorOpen ? "true" : "false",
    state.isArmed ? "true" : "false",
    state.offlineMode ? "true" : "false",
    state.sensorFailure ? "true" : "false"
  );

  return mqttClient.publish(telemetryTopic.c_str(), payload);
}

bool NetworkManager::publishAlarm(const String &reason, const SystemState &state) {
  if (!mqttClient.connected()) {
    return false;
  }

  char payload[256];
  snprintf(
    payload,
    sizeof(payload),
    "{\"deviceId\":\"%s\",\"roomId\":\"%s\",\"token\":\"%s\",\"reason\":\"%s\",\"armed\":%s}",
    DEVICE_ID,
    ROOM_ID,
    DEVICE_TOKEN,
    reason.c_str(),
    state.isArmed ? "true" : "false"
  );

  return mqttClient.publish(alarmTopic.c_str(), payload);
}

bool NetworkManager::publishHeartbeat(const SystemState &state) {
  if (!mqttClient.connected()) {
    return false;
  }

  char payload[256];
  snprintf(
    payload,
    sizeof(payload),
    "{\"deviceId\":\"%s\",\"roomId\":\"%s\",\"token\":\"%s\",\"wifi\":%s,\"mqtt\":%s,\"offline\":%s}",
    DEVICE_ID,
    ROOM_ID,
    DEVICE_TOKEN,
    state.wifiConnected ? "true" : "false",
    state.mqttConnected ? "true" : "false",
    state.offlineMode ? "true" : "false"
  );

  return mqttClient.publish(heartbeatTopic.c_str(), payload);
}

bool NetworkManager::publishStatus(const String &status, const SystemState &state) {
  if (!mqttClient.connected()) {
    return false;
  }

  char payload[256];
  snprintf(
    payload,
    sizeof(payload),
    "{\"deviceId\":\"%s\",\"roomId\":\"%s\",\"token\":\"%s\",\"status\":\"%s\",\"armed\":%s,\"offline\":%s}",
    DEVICE_ID,
    ROOM_ID,
    DEVICE_TOKEN,
    status.c_str(),
    state.isArmed ? "true" : "false",
    state.offlineMode ? "true" : "false"
  );

  return mqttClient.publish(statusTopic.c_str(), payload);
}

void NetworkManager::setCommandHandler(void (*handler)(const String&)) {
  externalCommandHandler = handler;
}

void NetworkManager::mqttCallback(char *topic, byte *payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (externalCommandHandler != nullptr) {
    externalCommandHandler(message);
  }
}

PubSubClient& NetworkManager::getClient() {
  return mqttClient;
}