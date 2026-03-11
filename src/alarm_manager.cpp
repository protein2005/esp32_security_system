#include "alarm_manager.h"
#include "pins.h"
#include <Arduino.h>

static const int BUZZER_CHANNEL = 0;
static const int BUZZER_RESOLUTION = 8;

void AlarmManager::begin() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void AlarmManager::trigger() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    ledcWriteTone(BUZZER_CHANNEL, 880);
    delay(100);

    digitalWrite(LED_PIN, LOW);
    ledcWriteTone(BUZZER_CHANNEL, 0);
    delay(100);
  }
}

void AlarmManager::stop() {
  digitalWrite(LED_PIN, LOW);
  ledcWriteTone(BUZZER_CHANNEL, 0);
}