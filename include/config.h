#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <IPAddress.h>

// --- OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// --- DHT ---
#define DHTTYPE DHT22

// --- WiFi ---
static const char *WIFI_SSID = "Wokwi-GUEST";
static const char *WIFI_PASSWORD = "";

// --- MQTT ---
// Локальний брокер на ПК, приклад: 192.168.0.105
static const IPAddress MQTT_SERVER(192, 168, 0, 106);
static const uint16_t MQTT_PORT = 1883;
static const char *MQTT_TOPIC_ROOT = "security";

// --- Firmware identity ---
static const char *DEVICE_TYPE = "esp32_security_node";
static const char *DEVICE_TOKEN = "room101_secure_token";
static const char *FIRMWARE_VERSION = "1.1.0";

// --- Thresholds ---
static constexpr float DEFAULT_TEMP_MIN_THRESHOLD = 18.0f;
static constexpr float DEFAULT_TEMP_MAX_THRESHOLD = 32.0f;
static constexpr float DEFAULT_HUMIDITY_MIN_THRESHOLD = 30.0f;
static constexpr float DEFAULT_HUMIDITY_MAX_THRESHOLD = 70.0f;

// --- Timing ---
static const unsigned long SENSOR_READ_INTERVAL = 1000;
static const unsigned long HEARTBEAT_INTERVAL = 5000;
static const unsigned long MQTT_RECONNECT_INTERVAL = 5000;
static const unsigned long MOTION_ALARM_HOLD_MS = 5000;
static const size_t OFFLINE_QUEUE_CAPACITY = 12;
static const uint16_t MQTT_PACKET_BUFFER_SIZE = 1024;

#endif
