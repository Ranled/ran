# B.A.N. BLE Protocol Documentation

## Overview

B.A.N. uses Bluetooth Low Energy (BLE) with a custom GATT service.
Communication uses JSON messages terminated by a NULL byte (`\x00`) for framing.
Large messages are split into 512-byte BLE packets and reassembled.

---

## UUIDs

| Name          | UUID                                   | Direction     | Properties |
|---------------|----------------------------------------|---------------|------------|
| **Service**   | `4fafc201-1fb5-459e-8fcc-c5c9c3319141` | —             | —          |
| **TX**        | `beb5483e-36e1-4688-b7f5-ea07361b26a8` | ESP32 → Web   | Notify     |
| **RX**        | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | Web → ESP32   | Write      |

> These UUIDs are also defined in:
> - `firmware/src/bluetooth/BLEManager.h` (ESP32)
> - `web/src/services/bluetooth.ts` (Web App)

---

## Message Framing

Each JSON message is terminated by a **NULL byte** (`\x00`, ASCII 0):

```
[JSON string bytes...][0x00]
```

Messages larger than **512 bytes** are split into chunks.
The receiver accumulates bytes until it sees `0x00`, then parses the JSON.

---

## Message Types

### Web → ESP32 (sent via RX characteristic)

#### 1. Chat Message

Send a text message to B.A.N. (commands or natural language).

```json
{
  "type": "chat",
  "message": "What is my RFID project?"
}
```

#### 2. Command

Request a specific action from the ESP32.

```json
{
  "type": "command",
  "command": "GET_STATUS"
}
```

Available commands:

| Command            | Effect                                     |
|--------------------|--------------------------------------------|
| `GET_STATUS`       | Returns current device status              |
| `GET_PERSONALITY`  | Returns personality name and content       |
| `GET_MEMORY`       | Returns full memory list                   |
| `RELOAD_PERSONALITY` | Reloads personality.txt from SD card     |
| `GET_CONVERSATION` | Returns current conversation (future)      |
| `CLEAR_CONVERSATION` | Clears current session conversation      |

#### 3. Save Memory

```json
{
  "type": "memory_save",
  "category": "PROJECT",
  "key": "CHARRMPASS",
  "value": "RFID vehicle entry and exit monitoring system"
}
```

#### 4. Delete Memory

```json
{
  "type": "memory_delete",
  "id": 3
}
```

#### 5. Search Memory

```json
{
  "type": "memory_search",
  "keyword": "RFID"
}
```

---

### ESP32 → Web (received via TX notifications)

#### 1. Response (chat reply)

```json
{
  "type": "response",
  "message": "CHARRMPASS is your RFID vehicle monitoring project."
}
```

#### 2. Status

```json
{
  "type": "status",
  "esp32": true,
  "bluetooth": true,
  "wifi": false,
  "sd": true,
  "ai": false,
  "memory": true,
  "freeHeap": 245000,
  "uptime": "5m 23s",
  "firmware": "BAN-ESP32-2.0",
  "memoryCount": 12
}
```

#### 3. Memory List

```json
{
  "type": "memory_list",
  "memories": [
    {
      "id": 1,
      "category": "PROJECT",
      "key": "CHARRMPASS",
      "value": "RFID vehicle monitoring system",
      "created": "2026-09-25"
    }
  ]
}
```

#### 4. Memory Saved Confirmation

```json
{
  "type": "memory_saved",
  "id": 6
}
```

#### 5. Memory Deleted Confirmation

```json
{
  "type": "memory_deleted",
  "id": 6
}
```

#### 6. Personality

```json
{
  "type": "personality",
  "name": "B.A.N.",
  "content": "Name: B.A.N.\nPersonality: Friendly, direct..."
}
```

#### 7. Error

```json
{
  "type": "error",
  "code": "SD_UNAVAILABLE",
  "message": "microSD card is unavailable."
}
```

Error codes:

| Code                   | Meaning                            |
|------------------------|------------------------------------|
| `PARSE_ERROR`          | Invalid JSON received              |
| `UNKNOWN_TYPE`         | Unknown message type               |
| `UNKNOWN_COMMAND`      | Unknown command                    |
| `MEMORY_SAVE_FAILED`   | Failed to write to memory.json     |
| `MEMORY_DELETE_FAILED` | Memory ID not found                |
| `MEMORY_INVALID`       | Missing key or value fields        |
| `MEMORY_PARSE_ERROR`   | memory.json is corrupted           |
| `SD_UNAVAILABLE`       | microSD card is not mounted        |

---

## Connection Flow

```
Browser                         ESP32
  |                               |
  | -- requestDevice() ---------> |  (BLE scan)
  |                               |
  | <-- Advertisement ----------- |  (BAN-ESP32)
  |                               |
  | -- GATT connect() ----------> |
  |                               |
  | -- getPrimaryService() -----> |
  | -- getCharacteristic(TX) ---> |
  | -- startNotifications() ----> |  (subscribe to TX)
  | -- getCharacteristic(RX) ---> |
  |                               |
  | -- write GET_STATUS --------> |
  | <-- status JSON + \x00 ------ |
  |                               |
  | -- write GET_PERSONALITY ---> |
  | <-- personality JSON + \x00 - |
  |                               |
  | -- write GET_MEMORY --------> |
  | <-- memory_list JSON + \x00 - |
  |                               |
  [CONNECTED AND READY]
```

---

## Browser Compatibility

Web Bluetooth is required for the web HCI to work.

| Browser | Web Bluetooth | Notes |
|---------|--------------|-------|
| Chrome (desktop) | ✅ | Fully supported |
| Edge (desktop) | ✅ | Fully supported |
| Chrome (Android) | ✅ | Supported |
| Safari (any) | ❌ | Not supported |
| Firefox (any) | ❌ | Not supported |
| Chrome iOS | ❌ | Not supported (iOS limitation) |

> Use Chrome or Edge on desktop/Android for the B.A.N. web interface.

---

## Slash Commands (processed locally by ESP32)

These are sent as `chat` messages but intercepted before reaching the AI:

| Command | Action |
|---------|--------|
| `/status` | Returns status message |
| `/memory` | Returns memory list |
| `/personality` | Returns personality |
| `/personality reload` | Reloads personality from SD |
| `/remember <text>` | Saves a NOTE memory |
| `/search <keyword>` | Searches memories |
| `/forget <id>` | Deletes memory by ID |

---

*B.A.N. BLE Protocol — v2.0 — 2026-09-25*
