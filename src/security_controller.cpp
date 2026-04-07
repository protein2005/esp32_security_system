#include "security_controller.h"
#include "config.h"
#include <ArduinoJson.h>

SecurityController* SecurityController::instance = nullptr;

void SecurityController::begin() {
  instance = this;

  Serial.begin(115200);

  sensors.begin();
  alarm.begin();

  if (!display.begin()) {
    Serial.println("OLED init failed");
    while (true) {
      delay(10);
    }
  }

  display.showStartup();

  network.begin();
  network.setCommandHandler(commandProxy);

  systemState.offlineMode = false;
  systemState.mqttConnected = false;
  systemState.wifiConnected = false;
  systemState.sensorFailure = false;
  systemState.alarmActive = false;
  systemState.alarmSilenced = false;
  systemState.queuedEvents = 0;
  systemState.tempMinThreshold = DEFAULT_TEMP_MIN_THRESHOLD;
  systemState.tempMaxThreshold = DEFAULT_TEMP_MAX_THRESHOLD;
  systemState.humidityMinThreshold = DEFAULT_HUMIDITY_MIN_THRESHOLD;
  systemState.humidityMaxThreshold = DEFAULT_HUMIDITY_MAX_THRESHOLD;

  loadPersistentState();

  Serial.println("ESP32 Security System Start");
  Serial.print("Room ID: ");
  Serial.println(ROOM_ID);
  Serial.print("Room Name: ");
  Serial.println(ROOM_NAME);
  Serial.print("Zone Type: ");
  Serial.println(ZONE_TYPE);
  Serial.print("Device ID: ");
  Serial.println(DEVICE_ID);
  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);
}

void SecurityController::update() {
  network.update(systemState);
  alarm.update();

  if (sensors.isArmButtonPressed()) {
    if (!systemState.isArmed) {
      setArmedState(true);

      Serial.println("Local action: ARMED");
      network.publishStatus("ARMED", systemState);
      logLocalAction("LOCAL_ARM", "Physical ARM button pressed");
    } else {
      Serial.println("Local action blocked: DISARM is remote-only");
      logLocalAction("LOCAL_DISARM_BLOCKED", "ARM button press ignored while already armed");
    }
  }

  if (sensors.isResetAlarmButtonPressed()) {
    logLocalAction("LOCAL_ALARM_SILENCE", "Physical RESET ALARM button pressed");
    resetAlarm(true);
  }

  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = now;

    sensorData = sensors.readData();
    systemState.sensorFailure = !sensorData.dhtOk;

    String currentAlarmReason = determineAlarmReason();
    if (currentAlarmReason.isEmpty()) {
      clearAlarmState(true);
    } else if (!systemState.alarmActive) {
      handleAlarm(currentAlarmReason);
    } else if (currentAlarmReason != systemState.activeAlarmReason) {
      systemState.alarmSilenced = false;
      handleAlarm(currentAlarmReason);
    }

    if (sensorData.dhtOk) {
      Serial.print("[");
      Serial.print(ROOM_ID);
      Serial.print("] T:");
      Serial.print(sensorData.temperature, 1);
      Serial.print("C H:");
      Serial.print(sensorData.humidity, 1);
      Serial.print("% | Motion:");
      Serial.print(sensorData.motionDetected);
      Serial.print(" | Door:");
      Serial.print(sensorData.doorOpen);
      Serial.print(" | Armed:");
      Serial.println(systemState.isArmed);
    } else {
      Serial.print("[");
      Serial.print(ROOM_ID);
      Serial.println("] DHT22 read error");
    }

    display.showSystemState(
      sensorData,
      systemState,
      systemState.alarmActive
        ? (systemState.alarmSilenced ? "ALARM: SILENCED" : "ALARM: ACTIVE")
        : (systemState.offlineMode ? "OFFLINE MODE" : "SYSTEM OK")
    );

    network.publishTelemetry(sensorData, systemState);
  }

  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = now;
    network.publishHeartbeat(systemState);
  }
}

void SecurityController::handleAlarm(const String &reason) {
  systemState.alarmActive = true;
  bool reasonChanged = systemState.activeAlarmReason != reason;
  systemState.alarmSilenced = false;
  systemState.activeAlarmReason = reason;

  Serial.print("!!! ALARM [");
  Serial.print(ROOM_ID);
  Serial.print("]: ");
  Serial.print(reason);
  Serial.println(" !!!");

  alarm.setVisualAlert(true);
  display.showAlarm(systemState.alarmSilenced ? "ALARM SILENCED" : "ALARM ACTIVE", reason);
  alarm.startSound();

  if (reasonChanged) {
    network.publishAlarm(reason, systemState);
    network.publishStatus("ALARM", systemState);
  }
}

void SecurityController::processCommands(const String &cmd) {
  Serial.print("MQTT CMD [");
  Serial.print(ROOM_ID);
  Serial.print("]: ");
  Serial.println(cmd);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cmd);
  if (error) {
    Serial.print("Command rejected: invalid JSON, ");
    Serial.println(error.c_str());
    return;
  }

  const char *token = "";
  if (doc["token"].is<const char*>()) {
    token = doc["token"];
  } else if (doc["deviceToken"].is<const char*>()) {
    token = doc["deviceToken"];
  }
  if (String(token) != DEVICE_TOKEN) {
    Serial.println("Command rejected: invalid token");
    return;
  }

  const char *targetRoom = ROOM_ID;
  if (doc["roomId"].is<const char*>()) {
    targetRoom = doc["roomId"];
  }
  if (String(targetRoom) != ROOM_ID) {
    Serial.println("Command ignored: wrong roomId");
    return;
  }

  const char *action = "";
  if (doc["action"].is<const char*>()) {
    action = doc["action"];
  }
  if (strcmp(action, "ARM") == 0) {
    setArmedState(true);
    network.publishStatus("ARMED", systemState);
  } else if (strcmp(action, "DISARM") == 0) {
    setArmedState(false);
    resetAlarm(false);
    clearAlarmState(false);
    network.publishStatus("DISARMED", systemState);
  } else if (strcmp(action, "RESET_ALARM") == 0) {
    resetAlarm(true);
  } else if (strcmp(action, "SET_THRESHOLDS") == 0) {
    float tempMin = !doc["tempMin"].isNull() ? doc["tempMin"].as<float>() : systemState.tempMinThreshold;
    float tempMax = !doc["tempMax"].isNull() ? doc["tempMax"].as<float>() : systemState.tempMaxThreshold;
    float humidityMin = !doc["humidityMin"].isNull() ? doc["humidityMin"].as<float>() : systemState.humidityMinThreshold;
    float humidityMax = !doc["humidityMax"].isNull() ? doc["humidityMax"].as<float>() : systemState.humidityMaxThreshold;

    if (doc["thresholds"].is<JsonObjectConst>()) {
      JsonObjectConst thresholds = doc["thresholds"].as<JsonObjectConst>();
      if (!thresholds["tempMin"].isNull()) {
        tempMin = thresholds["tempMin"].as<float>();
      }
      if (!thresholds["tempMax"].isNull()) {
        tempMax = thresholds["tempMax"].as<float>();
      }
      if (!thresholds["humidityMin"].isNull()) {
        humidityMin = thresholds["humidityMin"].as<float>();
      }
      if (!thresholds["humidityMax"].isNull()) {
        humidityMax = thresholds["humidityMax"].as<float>();
      }
    }

    if (updateThresholds(tempMin, tempMax, humidityMin, humidityMax)) {
      network.publishStatus("THRESHOLDS_UPDATED", systemState);
    } else {
      Serial.println("Command rejected: invalid threshold values");
    }
  } else {
    Serial.println("Command ignored: unsupported action");
  }
}

void SecurityController::setArmedState(bool armed) {
  if (systemState.isArmed == armed) {
    return;
  }

  systemState.isArmed = armed;
  saveArmedState();
}

void SecurityController::loadPersistentState() {
  preferences.begin("security", false);
  systemState.isArmed = preferences.getBool("isArmed", false);
  systemState.tempMinThreshold = preferences.getFloat("tempMin", DEFAULT_TEMP_MIN_THRESHOLD);
  systemState.tempMaxThreshold = preferences.getFloat("tempMax", DEFAULT_TEMP_MAX_THRESHOLD);
  systemState.humidityMinThreshold = preferences.getFloat("humMin", DEFAULT_HUMIDITY_MIN_THRESHOLD);
  systemState.humidityMaxThreshold = preferences.getFloat("humMax", DEFAULT_HUMIDITY_MAX_THRESHOLD);
}

void SecurityController::saveArmedState() {
  preferences.putBool("isArmed", systemState.isArmed);
}

void SecurityController::saveThresholdState() {
  preferences.putFloat("tempMin", systemState.tempMinThreshold);
  preferences.putFloat("tempMax", systemState.tempMaxThreshold);
  preferences.putFloat("humMin", systemState.humidityMinThreshold);
  preferences.putFloat("humMax", systemState.humidityMaxThreshold);
}

bool SecurityController::updateThresholds(float tempMin, float tempMax, float humidityMin, float humidityMax) {
  if (tempMin >= tempMax || humidityMin >= humidityMax) {
    return false;
  }

  if (tempMin < -40.0f || tempMax > 125.0f || humidityMin < 0.0f || humidityMax > 100.0f) {
    return false;
  }

  systemState.tempMinThreshold = tempMin;
  systemState.tempMaxThreshold = tempMax;
  systemState.humidityMinThreshold = humidityMin;
  systemState.humidityMaxThreshold = humidityMax;
  saveThresholdState();

  Serial.print("Thresholds updated: T[");
  Serial.print(systemState.tempMinThreshold, 1);
  Serial.print(", ");
  Serial.print(systemState.tempMaxThreshold, 1);
  Serial.print("] H[");
  Serial.print(systemState.humidityMinThreshold, 1);
  Serial.print(", ");
  Serial.print(systemState.humidityMaxThreshold, 1);
  Serial.println("]");
  return true;
}

bool SecurityController::isTemperatureOutOfRange() const {
  return sensorData.temperature < systemState.tempMinThreshold ||
         sensorData.temperature > systemState.tempMaxThreshold;
}

bool SecurityController::isHumidityOutOfRange() const {
  return sensorData.humidity < systemState.humidityMinThreshold ||
         sensorData.humidity > systemState.humidityMaxThreshold;
}

String SecurityController::determineAlarmReason() const {
  if (!sensorData.dhtOk) {
    return "SENSOR_FAILURE";
  }

  if (isTemperatureOutOfRange()) {
    return "TEMP_OUT_OF_RANGE";
  }

  if (isHumidityOutOfRange()) {
    return "HUMIDITY_OUT_OF_RANGE";
  }

  if (systemState.isArmed) {
    if (sensorData.doorOpen) {
      return "DOOR_OPEN";
    }

    if (sensorData.motionDetected) {
      return "MOTION";
    }
  }

  return "";
}

void SecurityController::clearAlarmState(bool publishStatusUpdate) {
  if (!systemState.alarmActive && !systemState.alarmSilenced && systemState.activeAlarmReason.isEmpty()) {
    return;
  }

  alarm.stopAll();
  systemState.alarmActive = false;
  systemState.alarmSilenced = false;
  systemState.activeAlarmReason = "";

  if (publishStatusUpdate) {
    network.publishStatus("ALARM_CLEARED", systemState);
  }
}

void SecurityController::resetAlarm(bool publishStatusUpdate) {
  if (!systemState.alarmActive) {
    alarm.stopSound();
    if (publishStatusUpdate) {
      network.publishStatus("ALARM_RESET", systemState);
    }
    return;
  }

  alarm.stopSound();
  alarm.setVisualAlert(true);
  systemState.alarmSilenced = true;

  display.showAlarm("ALARM SILENCED", systemState.activeAlarmReason);

  if (publishStatusUpdate) {
    network.publishStatus("ALARM_RESET", systemState);
  }
}

void SecurityController::logLocalAction(const String &eventName, const String &details) {
  network.publishEvent(eventName, "local", details, systemState);
}

void SecurityController::commandProxy(const String &cmd) {
  if (instance != nullptr) {
    instance->processCommands(cmd);
  }
}
