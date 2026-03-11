#include "security_controller.h"
#include "config.h"

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

  systemState.isArmed = false;
  systemState.offlineMode = false;
  systemState.mqttConnected = false;
  systemState.wifiConnected = false;
  systemState.sensorFailure = false;

  Serial.println("ESP32 Security System Start");
  Serial.print("Room ID: ");
  Serial.println(ROOM_ID);
  Serial.print("Device ID: ");
  Serial.println(DEVICE_ID);
}

void SecurityController::update() {
  network.update(systemState);

  if (sensors.isArmButtonPressed()) {
    systemState.isArmed = !systemState.isArmed;

    Serial.print("Статус змінено: ");
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
      Serial.println("] Помилка читання DHT22");
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
  Serial.print("!!! ТРИВОГА [");
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

  if (cmd.indexOf("\"token\":\"") == -1 || cmd.indexOf(DEVICE_TOKEN) == -1) {
    Serial.println("Команда відхилена: невірний token");
    return;
  }

  if (cmd.indexOf("\"action\":\"ARM\"") != -1) {
    systemState.isArmed = true;
    network.publishStatus("ARMED", systemState);
  } else if (cmd.indexOf("\"action\":\"DISARM\"") != -1) {
    systemState.isArmed = false;
    alarm.stop();
    network.publishStatus("DISARMED", systemState);
  } else if (cmd.indexOf("\"action\":\"RESET_ALARM\"") != -1) {
    alarm.stop();
    network.publishStatus("ALARM_RESET", systemState);
  }
}

void SecurityController::commandProxy(const String &cmd) {
  if (instance != nullptr) {
    instance->processCommands(cmd);
  }
}