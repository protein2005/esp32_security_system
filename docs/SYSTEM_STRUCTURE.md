# Структура Backend та Frontend

## 1. Backend Structure

Нижче рекомендована структура backend-проєкту.

```text
backend/
  src/
    main.ts
    app.module.ts

    common/
      dto/
      enums/
      guards/
      interceptors/
      pipes/
      utils/

    config/
      config.module.ts
      mqtt.config.ts
      database.config.ts
      auth.config.ts

    modules/
      auth/
        auth.controller.ts
        auth.service.ts
        auth.module.ts
        dto/

      users/
        users.controller.ts
        users.service.ts
        users.module.ts
        entities/

      devices/
        devices.controller.ts
        devices.service.ts
        devices.module.ts
        dto/
        entities/

      rooms/
        rooms.controller.ts
        rooms.service.ts
        rooms.module.ts
        dto/
        entities/

      telemetry/
        telemetry.controller.ts
        telemetry.service.ts
        telemetry.module.ts
        entities/

      alarms/
        alarms.controller.ts
        alarms.service.ts
        alarms.module.ts
        entities/

      events/
        events.controller.ts
        events.service.ts
        events.module.ts
        entities/

      commands/
        commands.controller.ts
        commands.service.ts
        commands.module.ts
        dto/
        entities/

      mqtt/
        mqtt.module.ts
        mqtt.service.ts
        mqtt.handlers.ts

      realtime/
        realtime.gateway.ts
        realtime.module.ts

      provisioning/
        provisioning.controller.ts
        provisioning.service.ts
        provisioning.module.ts

      state/
        state.service.ts
        state.module.ts

    prisma/
      schema.prisma
      migrations/
```

## 2. Backend Modules та відповідальність

### `mqtt`

Відповідає за:

- підключення до broker
- subscribe на topics
- dispatch MQTT payload у відповідні сервіси

### `devices`

Відповідає за:

- device registry
- список `UNPROVISIONED`
- деталі пристрою
- останній seen/status

### `provisioning`

Відповідає за:

- прив'язку device -> room
- відправку MQTT-команди `PROVISION`
- оновлення binding state

### `rooms`

Відповідає за:

- CRUD кімнат
- metadata (`roomId`, `roomName`, `zoneType`)
- сторінку room overview

### `telemetry`

Відповідає за:

- історію telemetry
- побудову графіків
- видачу останніх сенсорних даних

### `alarms`

Відповідає за:

- активні тривоги
- історію тривог
- їхній життєвий цикл

### `events`

Відповідає за:

- локальні події
- provisioning events
- audit trail

### `commands`

Відповідає за:

- REST API для команд
- валідацію та логування команд
- publish у MQTT

### `state`

Відповідає за:

- агрегований current state кімнати
- швидку видачу даних для dashboard

### `realtime`

Відповідає за:

- WebSocket / Socket.IO
- live updates для frontend

## 3. Рекомендована Prisma model / DB логіка

### devices

- `id`
- `deviceId`
- `deviceType`
- `firmwareVersion`
- `isProvisioned`
- `lastSeenAt`
- `lastStatus`
- `createdAt`
- `updatedAt`

### rooms

- `id`
- `roomId`
- `roomName`
- `zoneType`
- `description`
- `createdAt`
- `updatedAt`

### deviceRoomBindings

- `id`
- `deviceId`
- `roomId`
- `isCurrent`
- `boundAt`
- `unboundAt`

### roomCurrentState

- `id`
- `roomId`
- `deviceId`
- `temperature`
- `humidity`
- `motion`
- `door`
- `armed`
- `alarmActive`
- `alarmReason`
- `alarmSilenced`
- `wifiOk`
- `mqttOk`
- `offlineMode`
- `updatedAt`

### telemetryHistory

- `id`
- `deviceId`
- `roomId`
- `temperature`
- `humidity`
- `motion`
- `door`
- `armed`
- `sensorFailure`
- `receivedAt`

### alarms

- `id`
- `deviceId`
- `roomId`
- `reason`
- `isActive`
- `triggeredAt`
- `silencedAt`
- `clearedAt`

### events

- `id`
- `deviceId`
- `roomId`
- `eventName`
- `source`
- `details`
- `createdAt`

### commands

- `id`
- `targetDeviceId`
- `targetRoomId`
- `action`
- `payloadJson`
- `requestedBy`
- `status`
- `createdAt`

## 4. Backend API Sitemap

### Auth

- `POST /auth/login`
- `POST /auth/refresh`
- `POST /auth/logout`

### Devices

- `GET /devices`
- `GET /devices/unprovisioned`
- `GET /devices/:deviceId`
- `POST /devices/:deviceId/provision`
- `POST /devices/:deviceId/factory-reset`

### Rooms

- `GET /rooms`
- `POST /rooms`
- `GET /rooms/:roomId`
- `PATCH /rooms/:roomId`
- `GET /rooms/:roomId/state`
- `GET /rooms/:roomId/telemetry`
- `GET /rooms/:roomId/alarms`
- `GET /rooms/:roomId/events`

### Commands

- `POST /rooms/:roomId/arm`
- `POST /rooms/:roomId/disarm`
- `POST /rooms/:roomId/reset-alarm`
- `POST /rooms/:roomId/thresholds`

### Dashboard

- `GET /dashboard/summary`
- `GET /dashboard/active-alarms`
- `GET /dashboard/online-status`

## 5. Frontend Structure

```text
frontend/
  src/
    main.tsx
    App.tsx

    app/
      router.tsx
      providers.tsx
      store.ts

    shared/
      api/
      components/
      hooks/
      layouts/
      lib/
      types/
      utils/

    features/
      auth/
      dashboard/
      devices/
      provisioning/
      rooms/
      telemetry/
      alarms/
      events/
      commands/

    pages/
      LoginPage.tsx
      DashboardPage.tsx
      DevicesPage.tsx
      DeviceDetailsPage.tsx
      ProvisioningPage.tsx
      RoomsPage.tsx
      RoomDetailsPage.tsx
      AlarmCenterPage.tsx
      EventsPage.tsx
      SettingsPage.tsx
```

## 6. Frontend Sitemap

### 1. Login

- `/login`

### 2. Dashboard

- `/dashboard`

Показує:

- загальну статистику
- online/offline кімнати
- active alarms
- останні події

### 3. Devices

- `/devices`
- `/devices/:deviceId`

Показує:

- всі знайдені пристрої
- unprovisioned devices
- firmware version
- last seen
- статус вузла

### 4. Provisioning

- `/provisioning`

Показує:

- список `UNPROVISIONED` ESP32
- форму прив'язки до `roomId`, `roomName`, `zoneType`

### 5. Rooms

- `/rooms`
- `/rooms/:roomId`

Показує:

- список кімнат
- статус кожної кімнати
- telemetry
- thresholds
- armed/disarmed
- active alarm reason

### 6. Alarm Center

- `/alarms`

Показує:

- активні тривоги
- історію тривог
- фільтри

### 7. Events / Audit

- `/events`

Показує:

- локальні події
- provisioning events
- command history
- factory reset history

### 8. Settings

- `/settings`

Показує:

- системні налаштування
- broker metadata
- threshold policies
- account settings

## 7. Структура сторінки Room Details

Сторінка `/rooms/:roomId` має містити такі секції:

### Header

- room name
- zone type
- online/offline
- deviceId

### Current status card

- armed/disarmed
- alarm active
- alarm reason
- alarm silenced

### Telemetry card

- temperature
- humidity
- motion
- door
- sensor failure

### Thresholds card

- temp min
- temp max
- humidity min
- humidity max
- форма редагування

### Actions card

- `ARM`
- `DISARM`
- `RESET ALARM`
- `FACTORY_RESET`

### History section

- mini telemetry chart
- last events
- last alarms

## 8. Основні UI-компоненти

### Shared components

- `Sidebar`
- `Topbar`
- `StatusBadge`
- `AlarmBadge`
- `MetricCard`
- `SectionCard`
- `Table`
- `Chart`
- `CommandButton`
- `Modal`
- `ConfirmDialog`

### Devices components

- `DeviceTable`
- `DeviceStatusPill`
- `ProvisionButton`

### Rooms components

- `RoomGrid`
- `RoomCard`
- `RoomStatePanel`
- `ThresholdForm`

### Alarms components

- `AlarmList`
- `AlarmTimeline`
- `AlarmFilters`

### Events components

- `EventTable`
- `EventDetailsDrawer`

## 9. Realtime логіка у frontend

Frontend повинен через WebSocket отримувати:

- новий `device_seen`
- оновлення current room state
- нову alarm подію
- alarm clear
- нові event записи

Тобто сторінки:

- dashboard
- rooms
- alarms
- events

мають оновлюватися без ручного refresh.

## 10. Мінімальний MVP

Щоб швидко запустити систему, достатньо зробити такий MVP:

### Backend MVP

- MQTT ingest
- devices registry
- provisioning API
- current room state
- REST commands

### Frontend MVP

- dashboard
- unprovisioned devices page
- room details page
- alarm center

## 11. Порядок реалізації

1. Backend MQTT module
2. Devices module
3. Provisioning module
4. Room current state
5. Command module
6. Frontend dashboard
7. Frontend provisioning
8. Frontend room details
9. Frontend alarm center
10. Realtime layer
