#include "device_config_manager.h"
#include "config.h"
#include <Arduino.h>
#include <esp_system.h>

void DeviceConfigManager::begin() {
  preferences.begin("devicecfg", false);
}

void DeviceConfigManager::load(FirmwareConstants &firmware, ProvisionedConfig &provisioned) {
  firmware.deviceToken = DEVICE_TOKEN;
  firmware.firmwareVersion = FIRMWARE_VERSION;
  firmware.deviceType = DEVICE_TYPE;

  String storedDeviceId = preferences.getString("deviceId", "");
  if (storedDeviceId.isEmpty()) {
    storedDeviceId = buildDeviceIdFromMac();
    preferences.putString("deviceId", storedDeviceId);
  }
  firmware.deviceId = storedDeviceId;

  provisioned.roomId = preferences.getString("roomId", "");
  provisioned.roomName = preferences.getString("roomName", "");
  provisioned.zoneType = preferences.getString("zoneType", "");
  provisioned.isProvisioned = !provisioned.roomId.isEmpty();
}

void DeviceConfigManager::saveProvisionedConfig(const ProvisionedConfig &provisioned) {
  preferences.putString("roomId", provisioned.roomId);
  preferences.putString("roomName", provisioned.roomName);
  preferences.putString("zoneType", provisioned.zoneType);
}

void DeviceConfigManager::clearProvisionedConfig() {
  preferences.remove("roomId");
  preferences.remove("roomName");
  preferences.remove("zoneType");
}

String DeviceConfigManager::buildDeviceIdFromMac() const {
  uint64_t mac = ESP.getEfuseMac();
  uint32_t suffix = static_cast<uint32_t>(mac & 0xFFFFFF);
  char deviceId[32];
  snprintf(deviceId, sizeof(deviceId), "esp32-%06lX", static_cast<unsigned long>(suffix));
  return String(deviceId);
}
