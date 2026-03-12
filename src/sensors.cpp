#include "sensors.h"
#include "pins.h"
#include "config.h"

Sensors::Sensors() : dht(DHT_PIN, DHTTYPE), lastArmBtnState(HIGH), lastResetBtnState(HIGH) {}

void Sensors::begin() {
  pinMode(PIR_PIN, INPUT);
  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(ARM_BTN_PIN, INPUT_PULLUP);
  pinMode(RESET_ALARM_BTN_PIN, INPUT_PULLUP);
  dht.begin();
}

SensorData Sensors::readData() {
  SensorData data;

  data.temperature = dht.readTemperature();
  data.humidity = dht.readHumidity();
  data.motionDetected = digitalRead(PIR_PIN);
  data.doorOpen = !digitalRead(DOOR_PIN);
  data.dhtOk = !(isnan(data.temperature) || isnan(data.humidity));

  return data;
}

bool Sensors::isArmButtonPressed() {
  return isButtonPressed(ARM_BTN_PIN, lastArmBtnState);
}

bool Sensors::isResetAlarmButtonPressed() {
  return isButtonPressed(RESET_ALARM_BTN_PIN, lastResetBtnState);
}

bool Sensors::isButtonPressed(uint8_t pin, int &lastState) {
  int btnState = digitalRead(pin);

  if (btnState == LOW && lastState == HIGH) {
    lastState = btnState;
    delay(200);
    return true;
  }

  lastState = btnState;
  return false;
}
