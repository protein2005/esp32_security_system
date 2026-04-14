#ifndef DEVICE_CONFIG_MANAGER_H
#define DEVICE_CONFIG_MANAGER_H

#include <Preferences.h>
#include "models.h"

class DeviceConfigManager {
public:
  void begin();
  void load(FirmwareConstants &firmware, ProvisionedConfig &provisioned);
  void saveProvisionedConfig(const ProvisionedConfig &provisioned);
  void clearProvisionedConfig();
  void clearAll();

private:
  Preferences preferences;

  String buildDeviceIdFromMac() const;
};

#endif
