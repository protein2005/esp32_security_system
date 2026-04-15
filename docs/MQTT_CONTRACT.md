# MQTT Contract

Цей документ є канонічним описом MQTT-контракту для `ESP32 Security System`.

Він потрібен як спільна точка інтеграції для:

- firmware
- backend
- frontend
- симуляції

Кодова істина для поточної реалізації:

- `src/network_manager.cpp` - topic routing і publish payloads
- `src/security_controller.cpp` - command handling і device-side behavior
- `include/config.h` - `MQTT_TOPIC_ROOT`, broker host/port

## 1. Topic model

Система працює у двох режимах:

- `device-level` до provisioning
- `room-level` після provisioning

Кореневий префікс topic:

```text
security
```

Значення береться з `MQTT_TOPIC_ROOT`.

## 2. Unprovisioned device

Поки пристрій не прив'язаний до кімнати, він працює через:

```text
security/devices/<deviceId>/...
```

### Publish

- `security/devices/<deviceId>/status`
- `security/devices/<deviceId>/heartbeat`

### Subscribe

- `security/devices/<deviceId>/cmd`

### Behavior

- пристрій публікує `status` і `heartbeat`
- `telemetry`, `alarm` і звичайні локальні `event` до provisioning не публікуються
- backend може знайти новий вузол і надіслати `PROVISION`

## 3. Provisioned device

Після успішного provisioning publish переходить на room-level topics:

```text
security/rooms/<roomId>/...
```

### Publish

- `security/rooms/<roomId>/telemetry`
- `security/rooms/<roomId>/status`
- `security/rooms/<roomId>/heartbeat`
- `security/rooms/<roomId>/alarm`
- `security/rooms/<roomId>/event`

### Subscribe

- `security/devices/<deviceId>/cmd`
- `security/rooms/<roomId>/cmd`

### Behavior

- device-level `cmd` зберігається навіть після provisioning
- room-level `cmd` додається після provisioning
- publish telemetry/alarm/event починається лише після provisioning

## 4. Shared payload fields

Більшість outbound payload містить такі поля:

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceType": "esp32_security_node",
  "firmwareVersion": "1.1.0",
  "provisioned": true,
  "roomId": "room101",
  "roomName": "Server Room 101",
  "zoneType": "server_room",
  "tempMinThreshold": 18.0,
  "tempMaxThreshold": 32.0,
  "humidityMinThreshold": 30.0,
  "humidityMaxThreshold": 70.0
}
```

Примітки:

- `roomId`, `roomName`, `zoneType` можуть бути порожніми до provisioning
- threshold fields формуються зі стану пристрою, а не з firmware constants

## 5. Outbound messages

## 5.1 Telemetry

Topic:

```text
security/rooms/<roomId>/telemetry
```

У поточній реалізації `telemetry` публікується лише для provisioned device.

Payload:

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceType": "esp32_security_node",
  "firmwareVersion": "1.1.0",
  "provisioned": true,
  "roomId": "room101",
  "roomName": "Server Room 101",
  "zoneType": "server_room",
  "tempMinThreshold": 18.0,
  "tempMaxThreshold": 32.0,
  "humidityMinThreshold": 30.0,
  "humidityMaxThreshold": 70.0,
  "eventType": "telemetry",
  "temp": 24.4,
  "hum": 46.2,
  "motion": false,
  "door": false,
  "armed": true,
  "offline": false,
  "sensorFailure": false,
  "alarmActive": false,
  "alarmSilenced": false,
  "activeAlarmReason": "",
  "dhtOk": true
}
```

## 5.2 Status

Topic:

```text
security/devices/<deviceId>/status
```

або після provisioning:

```text
security/rooms/<roomId>/status
```

Payload shape:

```json
{
  "eventType": "status",
  "status": "ONLINE",
  "armed": false,
  "offline": false,
  "sensorFailure": false,
  "alarmActive": false,
  "alarmSilenced": false,
  "activeAlarmReason": "",
  "queuedEvents": 0
}
```

Типові значення `status`:

- `UNPROVISIONED`
- `PROVISIONED`
- `ONLINE`
- `ARMED`
- `DISARMED`
- `ALARM`
- `ALARM_RESET`
- `ALARM_CLEARED`
- `THRESHOLDS_UPDATED`
- `FACTORY_RESET`

## 5.3 Heartbeat

Topic:

```text
security/devices/<deviceId>/heartbeat
```

або після provisioning:

```text
security/rooms/<roomId>/heartbeat
```

Payload shape:

```json
{
  "eventType": "heartbeat",
  "wifi": true,
  "mqtt": true,
  "offline": false,
  "armed": false,
  "alarmActive": false,
  "alarmSilenced": false,
  "activeAlarmReason": "",
  "queuedEvents": 0
}
```

## 5.4 Alarm

Topic:

```text
security/rooms/<roomId>/alarm
```

У поточній реалізації `alarm` публікується лише для provisioned device.

Payload shape:

```json
{
  "eventType": "alarm",
  "reason": "DOOR_OPEN",
  "armed": true,
  "offline": false,
  "alarmActive": true,
  "alarmSilenced": false
}
```

Можливі `reason`:

- `SENSOR_FAILURE`
- `TEMP_OUT_OF_RANGE`
- `HUMIDITY_OUT_OF_RANGE`
- `DOOR_OPEN`
- `MOTION`

## 5.5 Event

Topic:

```text
security/rooms/<roomId>/event
```

У поточній реалізації `event` публікується лише для provisioned device.

Payload shape:

```json
{
  "eventType": "event",
  "eventName": "DEVICE_PROVISIONED",
  "source": "remote",
  "details": "Device was provisioned from backend",
  "armed": false,
  "offline": false,
  "alarmActive": false,
  "alarmSilenced": false,
  "activeAlarmReason": ""
}
```

Типові `eventName`:

- `DEVICE_PROVISIONED`
- `LOCAL_ARM`
- `LOCAL_ALARM_SILENCE`
- `LOCAL_DISARM_BLOCKED`
- `FACTORY_RESET`

Примітки:

- локальні дії публікуються тільки після provisioning
- `DEVICE_PROVISIONED` публікується одразу після успішного `PROVISION`, вже в room-level `event`
- `FACTORY_RESET` теж публікується з room-level topic до перезапуску пристрою

## 6. Inbound commands

Команди надсилаються в:

```text
security/devices/<deviceId>/cmd
```

або після provisioning також у:

```text
security/rooms/<roomId>/cmd
```

### Required validation

Перед виконанням команда проходить перевірки:

- JSON має бути валідним
- `token` або `deviceToken` має збігатися з `DEVICE_TOKEN`
- `deviceId`, якщо переданий, має збігатися з локальним `deviceId`
- `roomId`, якщо переданий для provisioned device, має збігатися з локальним `roomId`

### Supported actions

- `PROVISION`
- `ARM`
- `DISARM`
- `RESET_ALARM`
- `FACTORY_RESET`
- `SET_THRESHOLDS`

## 6.1 PROVISION

Payload:

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "PROVISION",
  "roomId": "room101",
  "roomName": "Server Room 101",
  "zoneType": "server_room"
}
```

Required fields:

- `roomId`
- `roomName`
- `zoneType`

Effects:

- зберігає provisioning config у `Preferences`
- переводить publish topics на `security/rooms/<roomId>/...`
- продовжує слухати device-level `cmd`
- публікує `DEVICE_PROVISIONED` у `event`
- публікує `PROVISIONED` у `status`

## 6.2 ARM

Payload:

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "ARM"
}
```

Effects:

- вмикає armed state
- зберігає `isArmed`
- публікує `ARMED` у `status`

## 6.3 DISARM

Payload:

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "DISARM"
}
```

Effects:

- вимикає armed state
- скидає активну тривогу
- публікує `DISARMED` у `status`

## 6.4 RESET_ALARM

Payload:

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "RESET_ALARM"
}
```

Effects:

- вимикає звук бузера
- залишає візуальну індикацію, якщо причина тривоги ще актуальна
- ставить `alarmSilenced = true`
- публікує `ALARM_RESET` у `status`
- якщо причина зникла, далі публікується `ALARM_CLEARED`

## 6.5 FACTORY_RESET

Payload:

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "FACTORY_RESET"
}
```

Effects:

- публікує `FACTORY_RESET` у `event`
- публікує `FACTORY_RESET` у `status`
- очищає persistent state і provisioning config
- перезапускає ESP32
- після рестарту вузол знову працює як `UNPROVISIONED`

## 6.6 SET_THRESHOLDS

Підтримуються два формати.

Flat payload:

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "SET_THRESHOLDS",
  "tempMin": 20,
  "tempMax": 28,
  "humidityMin": 35,
  "humidityMax": 60
}
```

Nested payload:

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "SET_THRESHOLDS",
  "thresholds": {
    "tempMin": 20,
    "tempMax": 28,
    "humidityMin": 35,
    "humidityMax": 60
  }
}
```

Validation rules:

- `tempMin < tempMax`
- `humidityMin < humidityMax`
- temperature range: `-40..125`
- humidity range: `0..100`

Effects:

- оновлює thresholds у runtime state
- зберігає їх у `Preferences`
- публікує `THRESHOLDS_UPDATED` у `status`

## 7. Device-side behavior

## 7.1 Local physical actions

Дозволені локально:

- ARM button
- RESET ALARM button

Заборонено локально:

- `DISARM`

Після provisioning локальні дії логуються через `event`.

## 7.2 Alarm lifecycle

Джерела тривоги:

- `SENSOR_FAILURE`
- `TEMP_OUT_OF_RANGE`
- `HUMIDITY_OUT_OF_RANGE`
- `DOOR_OPEN`
- `MOTION`

Особливості:

- `DOOR_OPEN` і `MOTION` активні лише коли система armed
- якщо причина тривоги змінюється, `alarmSilenced` скидається в `false`
- коли причина зникає, пристрій публікує `ALARM_CLEARED`

## 7.3 Offline queue

Якщо MQTT publish не вдається:

- повідомлення потрапляє в локальну чергу
- `offlineMode = true`
- `mqttConnected = false`
- після перепідключення черга автоматично flush-иться

Поточний ліміт:

```text
OFFLINE_QUEUE_CAPACITY = 12
```

## 8. Integration notes for backend/frontend

- Для discovery unprovisioned device достатньо слухати `security/devices/+/status` і `security/devices/+/heartbeat`.
- Для бойового режиму backend має слухати `security/rooms/+/telemetry`, `status`, `heartbeat`, `alarm`, `event`.
- Для команд краще вважати `deviceId` primary target, а `roomId` - додатковим routing layer після provisioning.
- Якщо payload розходиться з цим документом, пріоритет має поточна firmware-реалізація, після чого документ треба оновити.
