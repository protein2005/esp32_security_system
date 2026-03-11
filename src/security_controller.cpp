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
  systemState.queuedEvents = 0;

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

  if (sensors.isArmButtonPressed()) {
    setArmedState(!systemState.isArmed);

    Serial.print("Status changed: ");
    Serial.println(systemState.isArmed ? "ARMED" : "DISARMED");

    network.publishStatus(systemState.isArmed ? "ARMED" : "DISARMED", systemState);
  }

  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = now;

    sensorData = sensors.readData();
    systemState.sensorFailure = !sensorData.dhtOk;

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
      systemState.offlineMode ? "OFFLINE MODE" : "SYSTEM OK"
    );

    network.publishTelemetry(sensorData, systemState);

    if (!sensorData.dhtOk) {
      handleAlarm("SENSOR_FAILURE");
    } else if (sensorData.temperature > FIRE_TEMP_THRESHOLD) {
      handleAlarm("FIRE_DANGER");
    }

    if (systemState.isArmed) {
      if (sensorData.doorOpen) {
        handleAlarm("DOOR_OPEN");
      }

      if (sensorData.motionDetected) {
        handleAlarm("MOTION");
      }
    }
  }

  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = now;
    network.publishHeartbeat(systemState);
  }
}

void SecurityController::handleAlarm(const String &reason) {
  Serial.print("!!! ALARM [");
  Serial.print(ROOM_ID);
  Serial.print("]: ");
  Serial.print(reason);
  Serial.println(" !!!");

  display.showAlarm(reason);
  alarm.trigger();

  network.publishAlarm(reason, systemState);
  network.publishStatus("ALARM", systemState);
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
    alarm.stop();
    network.publishStatus("DISARMED", systemState);
  } else if (strcmp(action, "RESET_ALARM") == 0) {
    alarm.stop();
    network.publishStatus("ALARM_RESET", systemState);
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
}

void SecurityController::saveArmedState() {
  preferences.putBool("isArmed", systemState.isArmed);
}

void SecurityController::commandProxy(const String &cmd) {
  if (instance != nullptr) {
    instance->processCommands(cmd);
  }
}
