# B.A.N. — Raian AI Network
## Web HCI + ESP32 + BLE + microSD Setup Guide — v2.0

---

## Quick Start

### Step 1 — Run the Web App

```powershell
cd web
npm install
npm run dev
```

Open Chrome or Edge and go to: **http://localhost:5173**

### Step 2 — Flash the ESP32

```powershell
cd firmware

# Switch to BLE firmware (v2.0):
# Rename src/main.cpp → src/main_classic.cpp
# Rename src/main_ble.cpp → src/main.cpp

pio run --target upload
pio device monitor --baud 115200
```

### Step 3 — Prepare the microSD

1. Format microSD as FAT32
2. Copy `sdcard/BAN/` folder to the root of the card
3. Insert card into the SPI SD module connected to the ESP32

### Step 4 — Connect

1. Open Chrome or Edge
2. Navigate to **http://localhost:5173**
3. Click **⬡ Connect B.A.N.**
4. Select **BAN-ESP32** in the browser's Bluetooth picker
5. Wait for **● Connected**

---

## Hardware Wiring

### SD Card Module (SPI)

| SD Module Pin | ESP32 Pin | Description |
|---------------|-----------|-------------|
| CS            | GPIO 5    | Chip Select |
| MOSI          | GPIO 23   | Master Out  |
| MISO          | GPIO 19   | Master In   |
| SCK           | GPIO 18   | Clock       |
| VCC           | 3.3V      | Power       |
| GND           | GND       | Ground      |

> **Important:** The SD module must run at **3.3V**. Do not use 5V.

### Power

The ESP32 is powered via the USB cable from your computer.
No external power supply is needed for basic operation.

---

## Project Structure

```
R.A.N/
├── web/                          ← React + Vite Web HCI
│   ├── public/ban-character.jpg  ← B.A.N. character image
│   ├── src/
│   │   ├── App.tsx               ← Root application
│   │   ├── services/bluetooth.ts ← Web Bluetooth API
│   │   ├── services/storage.ts   ← Browser localStorage cache
│   │   ├── hooks/useBluetooth.ts ← BLE connection hook
│   │   ├── hooks/useChat.ts      ← Chat message management
│   │   ├── hooks/useBanStatus.ts ← Device status hook
│   │   ├── types/protocol.ts     ← JSON protocol types
│   │   └── components/
│   │       ├── CharacterPanel    ← B.A.N. character + status
│   │       ├── ChatWindow        ← Chat interface + input
│   │       ├── MemoryPanel       ← Memory viewer/editor
│   │       └── SettingsPanel     ← Settings + debug panel
│   └── package.json
│
├── firmware/
│   ├── src/
│   │   ├── main.cpp              ← v1.0 (Bluetooth Classic)
│   │   ├── main_ble.cpp          ← v2.0 (BLE — rename to use)
│   │   ├── bluetooth/
│   │   │   ├── BLEManager.h/cpp  ← BLE GATT server (v2.0)
│   │   │   └── BluetoothManager  ← Classic SPP (v1.0)
│   │   ├── storage/SDManager     ← microSD file I/O
│   │   ├── memory/MemoryManager  ← memory.json CRUD
│   │   ├── personality/          ← personality.txt loading
│   │   ├── commands/             ← CLI command processor
│   │   ├── system/SystemManager  ← Uptime, heap tracking
│   │   └── ai/AIClient           ← AI backend stub
│   └── platformio.ini
│
├── sdcard/BAN/
│   ├── personality.txt           ← Editable personality
│   ├── identity.txt              ← Name and version
│   ├── memory.json               ← Persistent memories
│   ├── settings.json             ← Device settings
│   └── knowledge/projects.txt    ← Knowledge base
│
└── docs/
    ├── BUILD_GUIDE.md            ← Wiring and setup
    ├── BLE_PROTOCOL.md           ← BLE UUID + protocol spec
    └── WEB_HCI.md                ← This file
```

---

## Browser Requirements

| Browser | Compatible | Notes |
|---------|-----------|-------|
| Chrome (desktop) | ✅ | Recommended |
| Edge (desktop) | ✅ | Fully supported |
| Chrome (Android) | ✅ | Mobile supported |
| Firefox | ❌ | Web Bluetooth not supported |
| Safari | ❌ | Web Bluetooth not supported |
| Chrome (iOS) | ❌ | iOS blocks Web Bluetooth |

> **Always use Chrome or Edge desktop/Android.**

---

## BLE UUIDs

| Name    | UUID |
|---------|------|
| Service | `4fafc201-1fb5-459e-8fcc-c5c9c3319141` |
| TX (Notify) | `beb5483e-36e1-4688-b7f5-ea07361b26a8` |
| RX (Write) | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` |

See [BLE_PROTOCOL.md](BLE_PROTOCOL.md) for full message format.

---

## Available Commands

Type any of these in the chat input:

| Command | Action |
|---------|--------|
| `/status` | Show device status |
| `/memory` | List all memories |
| `/remember <text>` | Save a new memory |
| `/search <keyword>` | Search memories |
| `/forget <id>` | Delete a memory |
| `/personality` | Show current personality |
| `/personality reload` | Reload from SD card |

---

## Switching Between v1.0 and v2.0

**v1.0 — Bluetooth Classic SPP (Serial Terminal)**
- Use `src/main.cpp` (as-is)
- Connect via Serial Bluetooth Terminal app on Android
- No web app needed

**v2.0 — BLE + Web Bluetooth (Web HCI)**
- Rename `src/main.cpp` → `src/main_classic.cpp`
- Rename `src/main_ble.cpp` → `src/main.cpp`
- Open `web/` in browser after running `npm run dev`

---

## Troubleshooting

### Web Bluetooth device picker shows nothing
- Make sure the ESP32 is powered and firmware is running
- Check Serial Monitor — should show `[BLE] Advertising started`
- Make sure you're using Chrome or Edge on desktop
- On some systems you may need to enable: `chrome://flags/#enable-web-bluetooth`

### Character image not loading
- Ensure `web/public/ban-character.jpg` exists
- Replace with your own image if needed (PNG/JPG/WebP)
- The image path in `CharacterPanel.tsx` can be updated

### SD card not found
- Check SPI wiring (5 wires minimum: CS, MOSI, MISO, SCK, GND)
- Confirm FAT32 format
- Confirm 3.3V power to SD module
- Check Serial Monitor for `[STORAGE]` messages

### Firmware compilation error
- Run `pio run` to see full error log
- Ensure `platformio.ini` uses `min_spiffs.csv` partition for BLE
- BLE + Classic BT cannot run simultaneously — choose one

---

*B.A.N. Web HCI — v2.0 — 2026-09-25*
