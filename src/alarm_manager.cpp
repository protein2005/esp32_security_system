#include "alarm_manager.h"
#include "pins.h"
#include <Arduino.h>

static const int BUZZER_CHANNEL = 0;
static const int BUZZER_RESOLUTION = 8;
static const uint16_t ALARM_TONES[] = { 740, 988, 659, 988, 0, 523, 784, 0 };
static const uint16_t ALARM_TONE_STEP_MS = 140;

void AlarmManager::begin() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void AlarmManager::setVisualAlert(bool enabled) {
  digitalWrite(LED_PIN, enabled ? HIGH : LOW);
}

void AlarmManager::startSound() {
  soundEnabled = true;
  toneStep = 0;
  lastToneChange = 0;
}

void AlarmManager::stopSound() {
  soundEnabled = false;
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void AlarmManager::stopAll() {
  soundEnabled = false;
  toneStep = 0;
  digitalWrite(LED_PIN, LOW);
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void AlarmManager::update() {
  if (!soundEnabled) {
    return;
  }

  unsigned long now = millis();
  if (lastToneChange != 0 && now - lastToneChange < ALARM_TONE_STEP_MS) {
    return;
  }

  ledcWriteTone(BUZZER_CHANNEL, ALARM_TONES[toneStep]);
  toneStep = (toneStep + 1) % (sizeof(ALARM_TONES) / sizeof(ALARM_TONES[0]));
  lastToneChange = now;
}
