#include "display_manager.h"
#include <Wire.h>
#include "config.h"
#include "pins.h"

DisplayManager::DisplayManager()
  : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

bool DisplayManager::begin() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  return display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

void DisplayManager::showStartup() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("SECURITY SYSTEM");
  display.setCursor(0, 20);
  display.println("INITIALIZING...");
  display.display();
}

void DisplayManager::showSystemState(const SensorData &data, const SystemState &state, const String &msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("SECURITY PANEL");
  display.println("----------------");

  display.setCursor(0, 18);
  display.println(state.isArmed ? "MODE: ARMED" : "MODE: DISARMED");

  display.setCursor(0, 30);
  if (data.dhtOk) {
    display.print("T:");
    display.print(data.temperature, 1);
    display.print(" H:");
    display.print(data.humidity, 1);
  } else {
    display.print("DHT SENSOR ERROR");
  }

  display.setCursor(0, 42);
  display.print("WiFi:");
  display.print(state.wifiConnected ? "OK " : "ERR");
  display.print(" MQTT:");
  display.print(state.mqttConnected ? "OK" : "ERR");

  display.setCursor(0, 54);
  display.println(msg);

  display.display();
}

void DisplayManager::showAlarm(const String &reason) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(18, 18);
  display.println("!!! ALARM !!!");
  display.setCursor(8, 38);
  display.println(reason);
  display.display();
}