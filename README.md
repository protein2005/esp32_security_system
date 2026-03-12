# ESP32 Security System

ESP32-вузол для охорони приміщення у форматі IoT-пристрою. Пристрій зчитує температуру, вологість, рух і стан дверей, публікує телеметрію та тривоги через MQTT, приймає JSON-команди та зберігає ключові параметри у flash через `Preferences`.

## Ідентифікація пристрою

Поточні значення за замовчуванням задані у [include/config.h](./include/config.h):

- `ROOM_ID`: `room101`
- `ROOM_NAME`: `Server Room 101`
- `ZONE_TYPE`: `server_room`
- `DEVICE_ID`: `esp32_room101`
- `DEVICE_TOKEN`: `room101_secure_token`
- `FIRMWARE_VERSION`: `1.1.0`

## Структура MQTT-топіків

Базовий префікс:

```text
security/rooms/<ROOM_ID>/
```

Для поточної конфігурації:

```text
security/rooms/room101/
```

### Топіки, які публікує ESP32

- `security/rooms/room101/telemetry`
  Регулярна телеметрія сенсорів і поточні пороги.
- `security/rooms/room101/status`
  Зміни стану, наприклад `ONLINE`, `ARMED`, `DISARMED`, `ALARM`, `THRESHOLDS_UPDATED`.
- `security/rooms/room101/heartbeat`
  Сигнал про стан підключення та працездатність вузла.
- `security/rooms/room101/alarm`
  Події тривоги та причина спрацювання.

### Топік, на який підписується ESP32

- `security/rooms/room101/cmd`
  Канал JSON-команд для керування режимом роботи та оновлення порогів.

## Спільні поля у вихідних JSON-повідомленнях

Ці поля присутні у вихідних MQTT payload:

- `deviceId`
- `roomId`
- `roomName`
- `zoneType`
- `firmwareVersion`
- `tempMinThreshold`
- `tempMaxThreshold`
- `humidityMinThreshold`
- `humidityMaxThreshold`

Додаткові поля залежать від конкретного топіка та типу події.

## Топік телеметрії

Топік:

```text
security/rooms/room101/telemetry
```

Приклад payload:

```json
{
  "deviceId": "esp32_room101",
  "roomId": "room101",
  "roomName": "Server Room 101",
  "zoneType": "server_room",
  "firmwareVersion": "1.1.0",
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
  "dhtOk": true
}
```

## Топік статусу

Топік:

```text
security/rooms/room101/status
```

Можливі значення `status`:

- `ONLINE`
- `ARMED`
- `DISARMED`
- `ALARM`
- `ALARM_RESET`
- `THRESHOLDS_UPDATED`

Приклад payload:

```json
{
  "deviceId": "esp32_room101",
  "roomId": "room101",
  "roomName": "Server Room 101",
  "zoneType": "server_room",
  "firmwareVersion": "1.1.0",
  "tempMinThreshold": 20.0,
  "tempMaxThreshold": 28.0,
  "humidityMinThreshold": 35.0,
  "humidityMaxThreshold": 60.0,
  "eventType": "status",
  "status": "THRESHOLDS_UPDATED",
  "armed": true,
  "offline": false,
  "sensorFailure": false,
  "queuedEvents": 0
}
```

## Топік heartbeat

Топік:

```text
security/rooms/room101/heartbeat
```

Приклад payload:

```json
{
  "deviceId": "esp32_room101",
  "roomId": "room101",
  "roomName": "Server Room 101",
  "zoneType": "server_room",
  "firmwareVersion": "1.1.0",
  "tempMinThreshold": 18.0,
  "tempMaxThreshold": 32.0,
  "humidityMinThreshold": 30.0,
  "humidityMaxThreshold": 70.0,
  "eventType": "heartbeat",
  "wifi": true,
  "mqtt": true,
  "offline": false,
  "armed": false,
  "queuedEvents": 0
}
```

## Топік тривог

Топік:

```text
security/rooms/room101/alarm
```

Можливі значення `reason`:

- `SENSOR_FAILURE`
- `TEMP_OUT_OF_RANGE`
- `HUMIDITY_OUT_OF_RANGE`
- `DOOR_OPEN`
- `MOTION`

Приклад payload:

```json
{
  "deviceId": "esp32_room101",
  "roomId": "room101",
  "roomName": "Server Room 101",
  "zoneType": "server_room",
  "firmwareVersion": "1.1.0",
  "tempMinThreshold": 18.0,
  "tempMaxThreshold": 32.0,
  "humidityMinThreshold": 30.0,
  "humidityMaxThreshold": 70.0,
  "eventType": "alarm",
  "reason": "MOTION",
  "armed": true,
  "offline": false
}
```

## Топік команд

Топік:

```text
security/rooms/room101/cmd
```

Усі команди мають містити:

- `roomId`
- `deviceToken`
- `action`

### ARM

```json
{
  "roomId": "room101",
  "deviceToken": "room101_secure_token",
  "action": "ARM"
}
```

### DISARM

```json
{
  "roomId": "room101",
  "deviceToken": "room101_secure_token",
  "action": "DISARM"
}
```

### RESET_ALARM

```json
{
  "roomId": "room101",
  "deviceToken": "room101_secure_token",
  "action": "RESET_ALARM"
}
```

### SET_THRESHOLDS

Варіант із плоским JSON:

```json
{
  "roomId": "room101",
  "deviceToken": "room101_secure_token",
  "action": "SET_THRESHOLDS",
  "tempMin": 20,
  "tempMax": 28,
  "humidityMin": 35,
  "humidityMax": 60
}
```

Варіант із вкладеним об'єктом:

```json
{
  "roomId": "room101",
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
- температура має бути в межах `-40..125`
- вологість має бути в межах `0..100`

Після успішного оновлення порогів ESP32 публікує:

- `status = THRESHOLDS_UPDATED` у `security/rooms/room101/status`

## Поведінка в offline mode

Якщо публікація в MQTT не вдалася:

- подія зберігається у локальну offline-чергу
- `offline` стає `true`
- після повторного підключення до MQTT накопичені події відправляються автоматично

Розмір черги задається в `include/config.h`:

- `OFFLINE_QUEUE_CAPACITY = 12`

## Збереження параметрів у Preferences

Пристрій зберігає у flash такі значення:

- `isArmed`
- `tempMinThreshold`
- `tempMaxThreshold`
- `humidityMinThreshold`
- `humidityMaxThreshold`

## Швидка перевірка через MQTT Explorer

Підписатись варто на:

- `security/rooms/room101/telemetry`
- `security/rooms/room101/status`
- `security/rooms/room101/heartbeat`
- `security/rooms/room101/alarm`

Публікувати команди потрібно в:

- `security/rooms/room101/cmd`

## Типовий сценарій демонстрації

1. Запустити симуляцію та дочекатися `ONLINE` у `status`.
2. Надіслати `ARM` у `.../cmd`.
3. Переконатися, що в `.../status` з'явився `ARMED`.
4. Надіслати `SET_THRESHOLDS` з вужчими межами.
5. Змінити умови в симуляції та перевірити `telemetry` і можливі `alarm`.
6. За потреби надіслати `DISARM` або `RESET_ALARM`.
