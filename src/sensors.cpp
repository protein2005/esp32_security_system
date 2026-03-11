#include "sensors.h"
#include "pins.h"
#include "config.h"

Sensors::Sensors() : dht(DHT_PIN, DHTTYPE), lastBtnState(HIGH) {}

void Sensors::begin() {
  pinMode(PIR_PIN, INPUT);
  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(ARM_BTN_PIN, INPUT_PULLUP);
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
  int btnState = digitalRead(ARM_BTN_PIN);

  if (btnState == LOW && lastBtnState == HIGH) {
    lastBtnState = btnState;
    delay(200);
    return true;
  }

  lastBtnState = btnState;
  return false;
}