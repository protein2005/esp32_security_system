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
  eventTopic = base + "/event";
  cmdTopic = base + "/cmd";
}

void NetworkManager::begin() {
  WiFi.mode(WIFI_STA);
  buildTopics();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(MQTT_PACKET_BUFFER_SIZE);
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

    flushOfflineQueue(state);
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

bool NetworkManager::publishTelemetry(const SensorData &data, SystemState &state) {
  JsonDocument doc;
  populateBasePayload(doc);
  populateThresholdPayload(doc, state);
  doc["eventType"] = "telemetry";
  doc["temp"] = data.temperature;
  doc["hum"] = data.humidity;
  doc["motion"] = data.motionDetected;
  doc["door"] = data.doorOpen;
  doc["armed"] = state.isArmed;
  doc["offline"] = state.offlineMode;
  doc["sensorFailure"] = state.sensorFailure;
  doc["alarmActive"] = state.alarmActive;
  doc["alarmSilenced"] = state.alarmSilenced;
  doc["activeAlarmReason"] = state.activeAlarmReason;
  doc["dhtOk"] = data.dhtOk;

  String payload;
  serializeJson(doc, payload);
  return publishOrQueue(telemetryTopic, payload, state);
}

bool NetworkManager::publishAlarm(const String &reason, SystemState &state) {
  JsonDocument doc;
  populateBasePayload(doc);
  populateThresholdPayload(doc, state);
  doc["eventType"] = "alarm";
  doc["reason"] = reason;
  doc["armed"] = state.isArmed;
  doc["offline"] = state.offlineMode;
  doc["alarmActive"] = state.alarmActive;
  doc["alarmSilenced"] = state.alarmSilenced;

  String payload;
  serializeJson(doc, payload);
  return publishOrQueue(alarmTopic, payload, state);
}

bool NetworkManager::publishHeartbeat(SystemState &state) {
  JsonDocument doc;
  populateBasePayload(doc);
  populateThresholdPayload(doc, state);
  doc["eventType"] = "heartbeat";
  doc["wifi"] = state.wifiConnected;
  doc["mqtt"] = state.mqttConnected;
  doc["offline"] = state.offlineMode;
  doc["armed"] = state.isArmed;
  doc["alarmActive"] = state.alarmActive;
  doc["alarmSilenced"] = state.alarmSilenced;
  doc["activeAlarmReason"] = state.activeAlarmReason;
  doc["queuedEvents"] = state.queuedEvents;

  String payload;
  serializeJson(doc, payload);
  return publishOrQueue(heartbeatTopic, payload, state);
}

bool NetworkManager::publishStatus(const String &status, SystemState &state) {
  JsonDocument doc;
  populateBasePayload(doc);
  populateThresholdPayload(doc, state);
  doc["eventType"] = "status";
  doc["status"] = status;
  doc["armed"] = state.isArmed;
  doc["offline"] = state.offlineMode;
  doc["sensorFailure"] = state.sensorFailure;
  doc["alarmActive"] = state.alarmActive;
  doc["alarmSilenced"] = state.alarmSilenced;
  doc["activeAlarmReason"] = state.activeAlarmReason;
  doc["queuedEvents"] = state.queuedEvents;

  String payload;
  serializeJson(doc, payload);
  return publishOrQueue(statusTopic, payload, state);
}

bool NetworkManager::publishEvent(const String &eventName, const String &source, const String &details, SystemState &state) {
  JsonDocument doc;
  populateBasePayload(doc);
  populateThresholdPayload(doc, state);
  doc["eventType"] = "event";
  doc["eventName"] = eventName;
  doc["source"] = source;
  doc["details"] = details;
  doc["armed"] = state.isArmed;
  doc["offline"] = state.offlineMode;
  doc["alarmActive"] = state.alarmActive;
  doc["alarmSilenced"] = state.alarmSilenced;
  doc["activeAlarmReason"] = state.activeAlarmReason;

  String payload;
  serializeJson(doc, payload);
  return publishOrQueue(eventTopic, payload, state);
}

void NetworkManager::setCommandHandler(void (*handler)(const String&)) {
  externalCommandHandler = handler;
}

void NetworkManager::mqttCallback(char *topic, byte *payload, unsigned int length) {
  (void)topic;
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

bool NetworkManager::publishOrQueue(const String &topic, const String &payload, SystemState &state) {
  Serial.print("MQTT publish attempt: topic=");
  Serial.print(topic);
  Serial.print(", bytes=");
  Serial.println(payload.length());

  if (mqttClient.connected()) {
    bool published = mqttClient.publish(topic.c_str(), payload.c_str());
    if (published) {
      state.mqttConnected = true;
      state.offlineMode = false;
      return true;
    }

    Serial.print("MQTT publish failed, client state=");
    Serial.println(mqttClient.state());
  }

  enqueueEvent(topic, payload, state);
  state.offlineMode = true;
  state.mqttConnected = false;
  return false;
}

void NetworkManager::enqueueEvent(const String &topic, const String &payload, SystemState &state) {
  size_t insertIndex = (queueHead + queueSize) % OFFLINE_QUEUE_CAPACITY;
  if (queueSize == OFFLINE_QUEUE_CAPACITY) {
    queueHead = (queueHead + 1) % OFFLINE_QUEUE_CAPACITY;
    insertIndex = (queueHead + queueSize - 1) % OFFLINE_QUEUE_CAPACITY;
  } else {
    queueSize++;
  }

  offlineQueue[insertIndex].topic = topic;
  offlineQueue[insertIndex].payload = payload;
  state.queuedEvents = queueSize;
}

void NetworkManager::flushOfflineQueue(SystemState &state) {
  while (queueSize > 0 && mqttClient.connected()) {
    QueuedEvent &event = offlineQueue[queueHead];
    if (!mqttClient.publish(event.topic.c_str(), event.payload.c_str())) {
      Serial.print("MQTT flush failed, client state=");
      Serial.println(mqttClient.state());
      state.mqttConnected = false;
      state.offlineMode = true;
      break;
    }

    event.topic = "";
    event.payload = "";
    queueHead = (queueHead + 1) % OFFLINE_QUEUE_CAPACITY;
    queueSize--;
  }

  state.queuedEvents = queueSize;
  if (queueSize == 0 && mqttClient.connected()) {
    state.mqttConnected = true;
    state.offlineMode = false;
  }
}

void NetworkManager::populateBasePayload(JsonDocument &doc) const {
  doc["deviceId"] = DEVICE_ID;
  doc["roomId"] = ROOM_ID;
  doc["roomName"] = ROOM_NAME;
  doc["zoneType"] = ZONE_TYPE;
  doc["firmwareVersion"] = FIRMWARE_VERSION;
}

void NetworkManager::populateThresholdPayload(JsonDocument &doc, const SystemState &state) const {
  doc["tempMinThreshold"] = state.tempMinThreshold;
  doc["tempMaxThreshold"] = state.tempMaxThreshold;
  doc["humidityMinThreshold"] = state.humidityMinThreshold;
  doc["humidityMaxThreshold"] = state.humidityMaxThreshold;
}
