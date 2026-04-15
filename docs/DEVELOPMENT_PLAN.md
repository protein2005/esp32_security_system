# План Розробки Backend та Frontend

## 1. Мета системи

Побудувати масштабовану систему моніторингу та охорони приміщень, у якій:

- один ESP32-вузол обслуговує одне приміщення
- багато ESP32 одночасно працюють через один MQTT broker
- один backend приймає, зберігає та нормалізує події
- frontend дозволяє адмініструвати пристрої, кімнати та тривоги

Система має підтримувати:

- provisioning нових пристроїв
- моніторинг кімнат у реальному часі
- керування режимом охорони
- зміну thresholds
- роботу з alarm/event history

## 2. Поточна роль ESP32

ESP32 уже реалізує:

- `UNPROVISIONED` режим
- генерацію стабільного `deviceId`
- `PROVISION`
- `ARM`
- `DISARM`
- `RESET_ALARM`
- `SET_THRESHOLDS`
- `FACTORY_RESET`
- `heartbeat`
- `status`
- `telemetry`
- `alarm`
- `event`

Отже backend і frontend повинні будуватись навколо вже наявного MQTT-контракту.

## 3. Режими роботи пристрою

### 3.1. UNPROVISIONED

Пристрій ще не прив'язаний до конкретної кімнати.

У цьому режимі:

- пристрій має `deviceId`
- backend бачить його як новий вузол
- доступні тільки `status` і `heartbeat`
- backend може виконати provisioning

### 3.2. PROVISIONED

Після прив'язки до кімнати пристрій:

- зберігає `roomId`, `roomName`, `zoneType`
- переходить на room-level topics для publish
- працює як повноцінний вузол приміщення

## 4. MQTT-модель

### 4.1. Для unregistered device

Publish:

- `security/devices/<deviceId>/status`
- `security/devices/<deviceId>/heartbeat`

Subscribe:

- `security/devices/<deviceId>/cmd`

### 4.2. Для registered device

Publish:

- `security/rooms/<roomId>/telemetry`
- `security/rooms/<roomId>/status`
- `security/rooms/<roomId>/heartbeat`
- `security/rooms/<roomId>/alarm`
- `security/rooms/<roomId>/event`

Subscribe:

- `security/devices/<deviceId>/cmd`
- `security/rooms/<roomId>/cmd`

## 5. Що має робити backend

### 5.1. MQTT ingest

Backend повинен:

- підписуватись на всі потрібні topics
- приймати JSON payload
- валідувати поля
- оновлювати поточний стан пристроїв і кімнат
- зберігати історичні дані
- пушити realtime-оновлення на frontend

### 5.2. Device registry

Backend повинен:

- зберігати всі знайдені `deviceId`
- бачити `UNPROVISIONED` пристрої
- дозволяти адміну прив'язати пристрій до кімнати

### 5.3. Command layer

Backend повинен відправляти MQTT-команди до пристроїв:

- `PROVISION`
- `ARM`
- `DISARM`
- `RESET_ALARM`
- `SET_THRESHOLDS`
- `FACTORY_RESET`

Кожна команда має бути:

- авторизована
- залогована
- пов'язана з користувачем/адміністратором

### 5.4. State aggregation

Backend повинен підтримувати current state для кімнати:

- online/offline
- armed/disarmed
- current telemetry
- current thresholds
- active alarm
- alarm reason
- alarm silenced

### 5.5. Audit та історія

Backend має окремо зберігати:

- command history
- provisioning history
- local action events
- alarm history
- telemetry history

## 6. Основні сутності в БД

Мінімальний набір:

- `devices`
- `rooms`
- `device_room_bindings`
- `room_current_state`
- `telemetry_history`
- `alarms`
- `events`
- `commands`
- `users`

## 7. План реалізації backend

### Етап 1. Базова інфраструктура

- підняти backend-проєкт
- підключити MQTT client
- підключити БД
- зробити базову структуру модулів

### Етап 2. Device discovery

- обробка `security/devices/+/status`
- обробка `security/devices/+/heartbeat`
- реєстрація нових `deviceId`
- список `UNPROVISIONED` пристроїв

### Етап 3. Provisioning

- API для прив'язки пристрою до кімнати
- публікація MQTT-команди `PROVISION`
- оновлення registry після `PROVISIONED`

### Етап 4. Telemetry/alarm ingest

- зберігання telemetry
- зберігання alarm history
- підтримка current room state

### Етап 5. Command API

- `ARM`
- `DISARM`
- `RESET_ALARM`
- `SET_THRESHOLDS`
- `FACTORY_RESET`

### Етап 6. Realtime updates

- WebSocket або Socket.IO
- push оновлень на frontend

### Етап 7. Auth та ролі

- login
- admin/operator roles
- журнал команд

## 8. План реалізації frontend

### Етап 1. Основний shell

- layout
- sidebar/topbar
- auth shell

### Етап 2. Dashboard

- список кімнат
- online/offline
- active alarms
- summary cards

### Етап 3. Unprovisioned devices

- список нових пристроїв
- картка пристрою
- форма provisioning

### Етап 4. Room details

- поточні показники
- armed/disarmed
- thresholds
- кнопки команд
- live state

### Етап 5. Alarm center

- активні тривоги
- історія тривог
- фільтри

### Етап 6. Event / audit page

- локальні події
- provisioning
- factory reset
- історія керування

## 9. Рекомендований стек

### Backend

- `Node.js`
- `NestJS` або `Express`
- `mqtt.js`
- `PostgreSQL`
- `Prisma`
- `Socket.IO` або `ws`

### Frontend

- `React`
- `Vite`
- `TypeScript`
- `React Query`
- `Socket.IO client` або WebSocket
- `Tailwind CSS` або `MUI`

## 10. Бажана послідовність розробки

Рекомендований порядок:

1. Backend MQTT ingest
2. Device registry
3. Provisioning API
4. Room current state
5. Command API
6. Frontend dashboard
7. Frontend provisioning page
8. Frontend room details
9. Alarm center
10. Audit/events

## 11. Результат після реалізації

Після завершення системи адміністратор зможе:

- побачити новий `deviceId`
- зареєструвати ESP32 без перепрошивки
- переглядати всі кімнати в одному інтерфейсі
- керувати охороною і порогами
- бачити тривоги та історію подій
- працювати з багатьма вузлами одночасно через єдиний backend
