# ESP32 Security System

ESP32-вузол для охорони приміщень у форматі IoT-пристрою. Прошивка зчитує температуру, вологість, рух і стан дверей, публікує телеметрію та тривоги через MQTT, приймає JSON-команди, зберігає критичні параметри в `Preferences` і підтримує модель масштабування на багато пристроїв без перепрошивки під кожну кімнату.

Окремий канонічний опис MQTT-контракту винесено в [docs/MQTT_CONTRACT.md](./docs/MQTT_CONTRACT.md).

## Призначення архітектури

Один ESP32 = один вузол моніторингу одного приміщення.

Система розрахована на:

- багато ESP32-пристроїв
- один backend
- один MQTT broker
- централізовану прив'язку пристрою до конкретної кімнати після першого запуску

Через це прошивка не повинна назавжди містити жорстко зашиті:

- `roomId`
- `roomName`
- `zoneType`

Ці поля задаються після реєстрації пристрою в системі та зберігаються локально.

## Нова модель конфігурації

Проєкт розділений на 3 рівні конфігурації.

### 1. Firmware constants

Це поля, які лишаються в прошивці та однакові для багатьох пристроїв:

- `DEVICE_TOKEN`
- `DEVICE_TYPE`
- `FIRMWARE_VERSION`
- `DEFAULT_TEMP_MIN_THRESHOLD`
- `DEFAULT_TEMP_MAX_THRESHOLD`
- `DEFAULT_HUMIDITY_MIN_THRESHOLD`
- `DEFAULT_HUMIDITY_MAX_THRESHOLD`
- Wi‑Fi/MQTT параметри
- `MQTT_TOPIC_ROOT`

Ці значення визначені в [include/config.h](./include/config.h).

### 2. Runtime config

Це поточний стан вузла під час роботи:

- `isArmed`
- `offlineMode`
- `mqttConnected`
- `wifiConnected`
- `sensorFailure`
- `alarmActive`
- `alarmSilenced`
- `activeAlarmReason`
- поточні threshold values

Ці поля описані в `SystemState` у [include/models.h](./include/models.h).

### 3. Provisioned config

Це конфігурація, яку пристрій отримує після реєстрації:

- `roomId`
- `roomName`
- `zoneType`
- `isProvisioned`

Ці поля зберігаються локально в `Preferences`, щоб після перезапуску пристрій уже знав, до якої кімнати він прив'язаний.

## Ідентичність пристрою

У прошивці більше не зберігається жорсткий `deviceId`.

Тепер `deviceId`:

- генерується автоматично з MAC ESP32
- є стабільним для конкретної плати
- зберігається в `Preferences`
- використовується як базовий ідентифікатор вузла в MQTT і backend

Генерація та збереження реалізовані в:

- [include/device_config_manager.h](./include/device_config_manager.h)
- [src/device_config_manager.cpp](./src/device_config_manager.cpp)

Приклад формату:

```text
esp32-1A2B3C
```

## Збереження в Preferences

У flash зберігаються:

- `deviceId`
- `roomId`
- `roomName`
- `zoneType`
- `isArmed`
- `tempMinThreshold`
- `tempMaxThreshold`
- `humidityMinThreshold`
- `humidityMaxThreshold`

## MQTT-модель

Використовується дворівнева схема:

- `device-level` до моменту прив'язки пристрою
- `room-level` після реєстрації

## MQTT topics для unregistered device

Якщо пристрій ще не прив'язаний до кімнати, він працює через:

```text
security/devices/<deviceId>/...
```

### Публікація

- `security/devices/<deviceId>/status`
- `security/devices/<deviceId>/heartbeat`

### Підписка

- `security/devices/<deviceId>/cmd`

У такому режимі backend може побачити новий вузол, який ще не прив'язаний до конкретної кімнати.

У режимі `UNPROVISIONED` пристрій працює як discovery-вузол, тому не передає:

- `telemetry`
- `alarm`
- звичайні локальні `event`

## MQTT topics для registered device

Після provisioning пристрій публікує робочі дані в room-based topics:

```text
security/rooms/<roomId>/...
```

### Публікація

- `security/rooms/<roomId>/telemetry`
- `security/rooms/<roomId>/status`
- `security/rooms/<roomId>/heartbeat`
- `security/rooms/<roomId>/alarm`
- `security/rooms/<roomId>/event`

### Підписка

Після реєстрації вузол слухає:

- `security/devices/<deviceId>/cmd`
- `security/rooms/<roomId>/cmd`

Це дозволяє:

- зберегти device-level керування
- додати room-level керування після прив'язки
- вмикати повний робочий режим лише після provisioning

## Спільні поля у вихідних JSON-повідомленнях

Більшість payload містить такі поля:

- `deviceId`
- `deviceType`
- `firmwareVersion`
- `provisioned`
- `roomId`
- `roomName`
- `zoneType`
- `tempMinThreshold`
- `tempMaxThreshold`
- `humidityMinThreshold`
- `humidityMaxThreshold`
- `alarmActive`
- `alarmSilenced`
- `activeAlarmReason`

## Телеметрія

Топік:

```text
security/devices/<deviceId>/telemetry
```

або після provisioning:

```text
security/rooms/<roomId>/telemetry
```

Приклад payload:

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

## Status

Топік:

```text
security/devices/<deviceId>/status
```

або після provisioning:

```text
security/rooms/<roomId>/status
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

## Heartbeat

Топік:

```text
security/devices/<deviceId>/heartbeat
```

або:

```text
security/rooms/<roomId>/heartbeat
```

Містить службову інформацію про:

- Wi‑Fi
- MQTT
- offline mode
- active alarm state
- queued events

## Alarm

Топік:

```text
security/devices/<deviceId>/alarm
```

або:

```text
security/rooms/<roomId>/alarm
```

Можливі `reason`:

- `SENSOR_FAILURE`
- `TEMP_OUT_OF_RANGE`
- `HUMIDITY_OUT_OF_RANGE`
- `DOOR_OPEN`
- `MOTION`

## Event

Топік:

```text
security/devices/<deviceId>/event
```

або:

```text
security/rooms/<roomId>/event
```

Використовується для службових подій, у тому числі:

- локальні фізичні дії
- події provisioning
- у майбутньому може використовуватись для audit trail remote-команд

Звичайні локальні події публікуються лише після provisioning.

Приклади `eventName`:

- `LOCAL_ARM`
- `LOCAL_ALARM_SILENCE`
- `LOCAL_DISARM_BLOCKED`
- `DEVICE_PROVISIONED`

## Команди

Команди надсилаються в:

```text
security/devices/<deviceId>/cmd
```

Після реєстрації додатково можна використовувати:

```text
security/rooms/<roomId>/cmd
```

Усі критичні команди проходять перевірку:

- `deviceToken`
- `deviceId`
- `roomId` для прив'язаного пристрою

## Provisioning

Provisioning дозволяє прив'язати універсальну прошивку до конкретного приміщення без перепрошивки.

Команда:

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

Після успішної обробки:

- пристрій зберігає `roomId`, `roomName`, `zoneType` в `Preferences`
- переходить на room-level publish topics
- продовжує слухати device-level `cmd`
- публікує `DEVICE_PROVISIONED` у `event`
- публікує `PROVISIONED` у `status`

## ARM / DISARM / RESET_ALARM

### ARM

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "ARM"
}
```

### DISARM

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "DISARM"
}
```

### RESET_ALARM

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "RESET_ALARM"
}
```

Поведінка `RESET_ALARM`:

- вимикає звук бузера
- LED лишається увімкненим, якщо причина тривоги ще існує
- `alarmSilenced` стає `true`
- якщо причина зникла, публікується `ALARM_CLEARED`
- якщо з'явилась нова причина тривоги, звук вмикається знову

### FACTORY_RESET

```json
{
  "deviceId": "esp32-1A2B3C",
  "deviceToken": "room101_secure_token",
  "action": "FACTORY_RESET"
}
```

Поведінка `FACTORY_RESET`:

- очищає збережені `Preferences`
- видаляє provisioning-конфігурацію
- скидає `isArmed`
- повертає threshold-и до значень за замовчуванням
- перезапускає ESP32

Після перезапуску пристрій знову працює як `UNPROVISIONED`.

## SET_THRESHOLDS

Приклад:

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

Або:

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

Правила валідації:

- `tempMin < tempMax`
- `humidityMin < humidityMax`
- температура в межах `-40..125`
- вологість в межах `0..100`

## Локальне фізичне керування

На платі локально дозволено:

- постановку на охорону
- приглушення звуку тривоги

Локально заборонено:

- `DISARM`

Локальні дії фіксуються через `event` topic.

Кнопки у схемі:

- `ARM ONLY`
- `RESET ALARM`

## Поведінка при першому старті

Якщо пристрій ще не зареєстрований:

- генерується `deviceId`
- вузол працює як `UNPROVISIONED`
- публікує лише `status` і `heartbeat` через `security/devices/<deviceId>/...`
- backend може знайти цей вузол і виконати provisioning

## Offline queue

Якщо MQTT publish не вдається:

- подія потрапляє в локальну offline-чергу
- `offline = true`
- після повторного підключення накопичені події відправляються автоматично

Поточний розмір черги:

- `OFFLINE_QUEUE_CAPACITY = 12`

## Безпека

У поточній моделі вже збережено базові перевірки:

- перевірка `deviceToken`
- перевірка `deviceId`
- перевірка `roomId` для прив'язаного пристрою

Це не повноцінна enterprise-схема автентифікації, але код уже підготовлений до серверної моделі реєстрації та подальшого посилення безпеки.

## Основні файли

- [include/config.h](./include/config.h) — firmware constants
- [include/models.h](./include/models.h) — базові моделі даних
- [include/device_config_manager.h](./include/device_config_manager.h) — керування device/provisioned config
- [src/device_config_manager.cpp](./src/device_config_manager.cpp) — генерація `deviceId` і робота з `Preferences`
- [include/network_manager.h](./include/network_manager.h) — MQTT API
- [src/network_manager.cpp](./src/network_manager.cpp) — topic routing і publish logic
- [include/security_controller.h](./include/security_controller.h) — головний контролер
- [src/security_controller.cpp](./src/security_controller.cpp) — business logic вузла

## Типовий сценарій роботи

1. Увімкнути новий ESP32.
2. Пристрій згенерує `deviceId` і почне працювати як `UNPROVISIONED`.
3. Backend знаходить новий `deviceId`.
4. Backend надсилає команду `PROVISION`.
5. ESP32 зберігає `roomId`, `roomName`, `zoneType`.
6. Після перезапуску або одразу після provisioning вузол працює як пристрій конкретної кімнати.
7. Надалі всі звичайні сценарії `ARM`, `DISARM`, `RESET_ALARM`, `SET_THRESHOLDS`, telemetry, heartbeat і alarm працюють без перепрошивки під нове приміщення.
