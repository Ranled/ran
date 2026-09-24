# R.A.N. — Raian AI Network
### ESP32 + microSD Local AI Conversational Companion

**R.A.N. (Raian AI Network)** is a 100% offline, local personal conversational AI companion running directly on an **ESP32 microcontroller with a microSD card** for permanent memory, coupled with an interactive animated Web HCI companion over **Bluetooth Low Energy (BLE)**.

No Cloud. No Raspberry Pi. No external LLM API required.

---

## 🌟 Core Features

- **Embedded Local AI Engine**: Intent classification across 18 intents, fuzzy memory search, keyword scoring, sliding context tracking with pronoun resolution (`"it"`, `"that"`, `"the project"`), and template-based natural response generation.
- **Permanent microSD Memory**: Structured knowledge, identity directives, personality rules, and dual-schema memories (`/RAN/memory.json`, `/RAN/knowledge.json`, `/RAN/responses.json`, `/RAN/personality.txt`, `/RAN/identity.txt`).
- **Safety Safeguards**: Memory erase protection with confirmation safeguard (`CONFIRM CLEAR`), atomic file writing via `.tmp` swap to prevent corruption on sudden power loss.
- **Web HCI Interface**:
  - Live animated character with 42 sprite frames across 10 distinct moods (`idle`, `wave`, `talking`, `happy`, `wondering`, `shock`, `angry`, `sad`, `walking`, `running`).
  - Text-to-Speech (TTS) voice responses that automatically animate the talking mouth.
  - Installable **Progressive Web App (PWA)** with offline service worker and dynamic character favicon.
  - Direct Web Bluetooth GATT client for seamless real-time telemetry and wireless chat.
- **Serial Debug Console**: Full interactive CLI over USB Serial at 115200 baud with slash commands (`/status`, `/memory`, `/remember <text>`, `/search <query>`, `/forget <id>`, `/clear_memory`, `/personality`, `/knowledge`, `/device`, `/storage`, `/reload`).

---

## 📐 Hardware Specifications & Pinout

| ESP32 Pin | microSD Module (SPI) | Description |
| :--- | :--- | :--- |
| **GPIO 5** | `CS` | Chip Select |
| **GPIO 18** | `SCK` | SPI Clock |
| **GPIO 23** | `MOSI` | Master Out Slave In |
| **GPIO 19** | `MISO` | Master In Slave Out |
| **3.3V / 5V** | `VCC` | Power (check regulator) |
| **GND** | `GND` | Common Ground |

---

## 📁 Repository Structure

```text
R.A.N/
├── character/                   # 42 pixel-art character sprite frames
├── docs/                        # Specifications, manuals, and design docs
├── firmware/
│   ├── RAN_ESP32/
│   │   └── RAN_ESP32.ino        # Primary Arduino IDE sketch
│   ├── BAN_ESP32/
│   │   └── BAN_ESP32.ino        # Synchronized mirror sketch
│   ├── platformio.ini           # PlatformIO project configuration
│   └── src/                     # Modular C++ codebase
├── sdcard/
│   └── RAN/                     # microSD root directory files
│       ├── identity.txt
│       ├── personality.txt
│       ├── settings.json
│       ├── memory.json
│       ├── knowledge.json
│       ├── responses.json
│       ├── commands.json
│       └── conversations/
└── web/                         # PWA Web HCI (Vite + React + TypeScript)
```

---

## 🚀 Getting Started

### 1. Preparing the microSD Card
1. Format a microSD card as **FAT32**.
2. Copy the contents of the [`sdcard/RAN/`](sdcard/RAN/) folder into a `/RAN/` folder at the root of the microSD card.
3. Insert the card into your ESP32 microSD adapter module.

### 2. Flashing the ESP32 Firmware
1. Open [`firmware/RAN_ESP32/RAN_ESP32.ino`](firmware/RAN_ESP32/RAN_ESP32.ino) in **Arduino IDE**.
2. Go to **Tools -> Manage Libraries** and install **ArduinoJson** (v6 or v7).
3. Under **Tools -> Board**, select **ESP32 Dev Module**.
4. Under **Tools -> Partition Scheme**, select **Huge APP (3MB No OTA/1MB SPIFFS)**.
5. Connect your ESP32 via USB and click **Upload**.
6. Open the Serial Monitor at **115200 baud** to see R.A.N. boot!

### 3. Running the Web HCI Companion
```bash
cd web
npm install
npm run dev
```
Open `http://localhost:5173/` in Google Chrome or Microsoft Edge, and click **Connect ESP32** to pair wirelessly over Bluetooth Low Energy.

---

## 📄 License
MIT License. Created by Raian (Master).
