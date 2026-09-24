# B.A.N. — Raian AI Network
## Complete Build Guide — Version 1.0

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Hardware Required](#2-hardware-required)
3. [Wiring Diagram](#3-wiring-diagram)
4. [Software Setup](#4-software-setup)
5. [microSD Card Preparation](#5-microsd-card-preparation)
6. [Uploading the Firmware](#6-uploading-the-firmware)
7. [Bluetooth Testing](#7-bluetooth-testing)
8. [Command Reference](#8-command-reference)
9. [Architecture Explanation](#9-architecture-explanation)
10. [Troubleshooting Guide](#10-troubleshooting-guide)
11. [Version Roadmap](#11-version-roadmap)

---

## 1. Project Overview

**B.A.N. (Raian AI Network)** is a personal embedded AI assistant.

| Component | Role |
|-----------|------|
| **ESP32** | Body — controller, Bluetooth, memory manager |
| **microSD** | Long-term memory — personality, memories, conversations |
| **External AI** | Brain — language model (phone, PC, Raspberry Pi, cloud) |

### Design Philosophy

- The ESP32 does **NOT** run a language model.
- The ESP32 handles **communication, memory, personality, and commands**.
- The AI brain can be **swapped or upgraded** without rewriting ESP32 firmware.
- The system works **offline** — memory and commands always available.

---

## 2. Hardware Required

### Minimum (Version 1.0)

| Item | Quantity | Notes |
|------|----------|-------|
| ESP32 development board | 1 | DOIT DevKit V1, ESP32-WROOM-32, or equivalent |
| microSD card module | 1 | SPI interface, 3.3V compatible |
| microSD card | 1 | 2GB to 32GB, FAT32 formatted |
| USB cable | 1 | Micro-USB or USB-C depending on board |
| USB power supply | 1 | 5V, 1A minimum |
| Jumper wires | 8+ | Dupont female-to-female |
| Breadboard | 1 | Optional but recommended |

### Optional (Future Versions)

| Item | Purpose | Version |
|------|---------|---------|
| OLED display (128x64, I2C) | Status display | v3.0 |
| Push button | Input control | v3.0 |
| Status LED | Visual feedback | v3.0 |
| Microphone module (I2S) | Voice input | v4.0 |
| Speaker + amplifier (I2S) | Voice output | v4.0 |
| Raspberry Pi | Local AI backend | v5.0 |

---

## 3. Wiring Diagram

### ESP32 to microSD Card Module (SPI)

```
ESP32 Pin       microSD Module Pin
--------------------------------------
3.3V    -----> VCC  (3.3V power)
GND     -----> GND  (common ground)
GPIO 5  -----> CS   (chip select)
GPIO 23 -----> MOSI (Master Out Slave In)
GPIO 19 -----> MISO (Master In Slave Out)
GPIO 18 -----> SCK  (clock)
```

> **IMPORTANT**: Use 3.3V — NOT 5V — for the SD module VCC.
> Most SD card modules have an onboard 3.3V regulator,
> but always check your specific module's datasheet.
> Connecting 5V directly to the SD card data lines can damage it.

### Visual Wiring Map

```
  ESP32 DevKit V1
  +-----------------------------+
  |  3V3 o---------------------> VCC  (SD Module)
  |  GND o---------------------> GND
  |  D5  o---------------------> CS
  |  D23 o---------------------> MOSI
  |  D19 o---------------------> MISO
  |  D18 o---------------------> SCK
  +-----------------------------+
```

### Notes on Pin Selection

- **GPIO 5 (CS)**: Can be changed in `SDManager.h` via `SD_CS_PIN`
- **GPIO 23/19/18**: Standard SPI pins on ESP32, do not usually conflict
- **GPIO 0 and 2**: Avoid — used for boot mode
- **GPIO 34, 35, 36, 39**: Input only — cannot be used for SPI

### Changing Pins

If you need different pins, edit `SDManager.h`:

```cpp
#define SD_CS_PIN    5    // Change this
#define SD_MOSI_PIN  23   // Change this
#define SD_MISO_PIN  19   // Change this
#define SD_SCK_PIN   18   // Change this
```

---

## 4. Software Setup

### Option A: PlatformIO (Recommended)

PlatformIO gives you proper dependency management and easy builds.

**Step 1**: Install VS Code
- Download: https://code.visualstudio.com/

**Step 2**: Install PlatformIO extension
- Open VS Code, go to Extensions (Ctrl+Shift+X)
- Search "PlatformIO IDE" and install it

**Step 3**: Open the project
- File > Open Folder > select `R.A.N/firmware/`
- PlatformIO will automatically install the `espressif32` platform and `ArduinoJson` library

**Step 4**: Build
- Click the checkmark (Build) button in the PlatformIO toolbar
- Or press Ctrl+Shift+P and choose "PlatformIO: Build"

**Step 5**: Upload
- Connect ESP32 via USB
- Click the arrow (Upload) button
- Or press Ctrl+Shift+P and choose "PlatformIO: Upload"

### Option B: Arduino IDE

**Step 1**: Install Arduino IDE 2.x
- Download: https://www.arduino.cc/en/software

**Step 2**: Add ESP32 board support
- File > Preferences > Additional Boards Manager URLs, add:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Tools > Board > Boards Manager > Search "esp32" by Espressif > Install

**Step 3**: Install libraries
- Sketch > Include Library > Manage Libraries
- Search and install: **ArduinoJson** by Benoit Blanchon (version 6.x)
- The SD library comes with the ESP32 Arduino core — no install needed

**Step 4**: Select board settings
- Tools > Board > ESP32 Arduino > **ESP32 Dev Module**
- Tools > Upload Speed > **921600**
- Tools > Port > (your COM port)

**Step 5**: Prepare the sketch
- Rename `main.cpp` to `BAN.ino`
- Move all `.cpp` and `.h` files into the same sketch folder
- Arduino IDE will compile them automatically

### Required Libraries

| Library | Version | Purpose | Source |
|---------|---------|---------|--------|
| ArduinoJson | 6.x | Parse memory.json and settings.json | Arduino Library Manager |
| BluetoothSerial | built-in | Bluetooth Classic SPP | ESP32 Arduino Core |
| SD | built-in | microSD card access | ESP32 Arduino Core |
| SPI | built-in | SPI bus communication | ESP32 Arduino Core |

---

## 5. microSD Card Preparation

### Step 1: Format the card

- Use FAT32 format
- On Windows: Right-click the SD card in File Explorer > Format > FAT32
- On Linux: `mkfs.fat -F32 /dev/sdX`
- Maximum supported card size: 32GB (FAT32 limit)

### Step 2: Copy the SD card files

Copy the entire contents of `R.A.N/sdcard/` to the root of your SD card.

The final structure on the SD card must be:

```
SD Card Root
+-- BAN/
    +-- personality.txt      (personality traits)
    +-- identity.txt         (name, version, purpose)
    +-- settings.json        (configuration)
    +-- memory.json          (memory records)
    +-- knowledge/
    |   +-- projects.txt     (project notes)
    +-- conversations/       (auto-created by ESP32 on boot)
    +-- logs/                (auto-created by ESP32 on boot)
```

The `conversations/` and `logs/` directories are created automatically
by the ESP32 on first boot if they are missing.

### Step 3: Customize personality.txt (optional)

Open `BAN/personality.txt` on the SD card and edit it to your preferences.
Changes take effect after sending `/personality reload` or rebooting the ESP32.

---

## 6. Uploading the Firmware

### Using PlatformIO (command line)

```bash
cd R.A.N/firmware
pio run --target upload
pio device monitor --baud 115200
```

### Using Arduino IDE

1. Open `BAN.ino`
2. Select the correct board and COM port
3. Click Upload (the arrow button)
4. Open Serial Monitor (Ctrl+Shift+M), set baud rate to 115200

### Expected Serial Output on First Boot

```
+======================================+
|        B.A.N. BOOTING...             |
|   Raian AI Network - ESP32 v1.0      |
+======================================+

[SYSTEM] System manager initialized.
[STORAGE] Initializing microSD... OK
[STORAGE] SD Card mounted successfully.
[PERSONALITY] Loading personality... OK
[PERSONALITY] personality.txt loaded.
[PERSONALITY] identity.txt loaded.
[PERSONALITY] Name: B.A.N.
[MEMORY] Initializing memory system... OK (5 records loaded)
[AI] AIClient initialized. Backend: NONE (offline mode)
[CMD] Command processor ready.
[BLUETOOTH] Starting Bluetooth... OK
[BLUETOOTH] Device name: BAN-Device
[BLUETOOTH] Waiting for connection...

+======================================+
|          B.A.N. ONLINE               |
+======================================+

--- B.A.N. STATUS ---
  SD Card:     OK
  Memory:      OK
  Personality: LOADED
  AI Backend:  UNAVAILABLE
  Bluetooth:   WAITING
  Free Heap:   XXXX bytes
```

---

## 7. Bluetooth Testing

### On Android

**Recommended apps:**
- **Serial Bluetooth Terminal** by Kai Morich (free on Play Store)
- **BlueTerm** (simple terminal app)

**Steps:**
1. Open phone Settings > Bluetooth
2. Scan for devices and find **"BAN-Device"**
3. Pair with it (PIN is usually 1234 or 0000, or no PIN required)
4. Open Serial Bluetooth Terminal
5. Connect to BAN-Device
6. Type a message and press Send

### On Windows/PC

**Recommended app:** PuTTY or any COM port terminal

**Steps:**
1. Settings > Bluetooth > Add device > BAN-Device
2. After pairing, open Device Manager to find the COM port number
3. Open PuTTY > Connection type: Serial > COM port > Speed: 115200 > Open
4. Type messages and press Enter

### First Connection Test

Type:
```
Hello
```

Expected response:
```
AI|I can't reach the AI backend right now.
Local memory and commands are still available.
Tip: Use /search <keyword> to look up stored information.
```

Then test a command:
```
/status
```

Expected response:
```
AI|=== B.A.N. STATUS ===

ESP32:       ONLINE
SD Card:     OK
Memory:      OK (5 records)
Personality: LOADED
AI Backend:  UNAVAILABLE
Free Heap:   XXXX bytes
Uptime:      3s
Chip Model:  ESP32-D0WDQ6
CPU Freq:    240 MHz
```

---

## 8. Command Reference

### System Commands

| Command | Description |
|---------|-------------|
| `/help` | Show all available commands |
| `/status` | Full system status report |
| `/time` | Show uptime since last boot |
| `/device` | ESP32 hardware information |
| `/storage` | SD card storage usage |
| `/reload` | Reload all SD card files |

### Memory Commands

| Command | Description | Example |
|---------|-------------|---------|
| `/memory` | List all stored memories | `/memory` |
| `/remember <text>` | Save a new memory | `/remember CHARRMPASS is my RFID project` |
| `/search <keyword>` | Search memories by keyword | `/search RFID` |
| `/forget <id>` | Delete memory by ID number | `/forget 3` |
| `/clear_memory` | Clear all memories (asks confirmation) | `/clear_memory` |
| `/clear_memory confirm` | Confirmed delete all memories | `/clear_memory confirm` |

### Personality Commands

| Command | Description |
|---------|-------------|
| `/personality` | Show personality summary |
| `/personality show` | Same as above |
| `/personality reload` | Reload personality.txt from SD card without rebooting |

### Message Protocol

The system uses a pipe character `|` as a prefix separator:

| Prefix | Direction | Meaning |
|--------|-----------|---------|
| `USER|` | Phone to ESP32 | User message |
| `AI|` | ESP32 to Phone | B.A.N. response |
| `SYSTEM|` | ESP32 to Phone | System notification |
| `MEMORY|` | ESP32 to Phone | Memory operation result |
| `ERROR|` | ESP32 to Phone | Error message |

The prefix is optional. You can send plain text and the ESP32 will handle it correctly.

---

## 9. Architecture Explanation

### How the Modules Work Together

```
USER (Phone/PC)
      |
      | Bluetooth SPP (text)
      |
      v
+-----------------------------------+
|             ESP32                 |
|                                   |
|  BluetoothManager                 |
|  (receives text, sends responses) |
|         |                         |
|  CommandProcessor                 |
|  (routes commands and messages)   |
|         |                         |
|  +------+------+                  |
|  |             |                  |
|  MemoryManager PersonalityManager |
|  |             |                  |
|  SDManager (microSD card)         |
|                                   |
|  AIClient (abstract AI layer)     |
|  (offline in v1.0 / HTTP in v2.0) |
+-----------------------------------+
      |
      | Wi-Fi HTTP (Stage 7+)
      |
      v
External AI Backend (phone / PC / Pi)
      |
      v
Language Model
```

### Module Responsibilities

| Module | File Location | What It Does |
|--------|---------------|-------------|
| `main.cpp` | `src/` | Boot sequence and main event loop |
| `BluetoothManager` | `src/bluetooth/` | Receive/send Bluetooth SPP messages |
| `SDManager` | `src/storage/` | All microSD file read/write operations |
| `MemoryManager` | `src/memory/` | Create, search, delete memory.json records |
| `PersonalityManager` | `src/personality/` | Load personality.txt and identity.txt |
| `CommandProcessor` | `src/commands/` | Parse commands and call the right handler |
| `SystemManager` | `src/system/` | Uptime tracking and periodic housekeeping |
| `AIClient` | `src/ai/` | Abstract interface to external AI backend |

### Non-Blocking Design

The main `loop()` function never uses `delay()`.
All timing uses `millis()`:

- Bluetooth bytes are read one byte at a time per loop iteration
- The SystemManager periodic task fires every 30 seconds via millis check
- The AIClient is designed for future async HTTP polling

---

## 10. Troubleshooting Guide

### SD Card Not Detected

**Serial shows:** `[STORAGE] Initializing microSD... FAILED`

**Check these things:**
1. Verify wiring: CS to GPIO 5, MOSI to GPIO 23, MISO to GPIO 19, SCK to GPIO 18
2. Confirm VCC connects to 3.3V, not 5V
3. Confirm the SD card is FAT32 formatted (not exFAT or NTFS)
4. Try the SD card in a computer to verify it is not corrupted
5. Try a different SD card
6. Try a slower SPI speed (add `SD.begin(SD_CS_PIN, SPI, 4000000)` to reduce to 4 MHz)

---

### Bluetooth Not Appearing During Phone Scan

**Symptoms:** "BAN-Device" not visible in phone Bluetooth scan

**Check:**
1. Confirm the ESP32 is powered (look for the power LED)
2. Open the Serial Monitor and confirm you see "B.A.N. ONLINE"
3. Confirm the correct COM port was selected before uploading
4. Some ESP32 boards require holding the BOOT button during the upload process
5. Bluetooth Classic (SPP) is different from BLE — make sure your phone is scanning for Classic devices

---

### Memory Not Saving

**Symptoms:** `/remember X` returns "Failed to save memory"

**Check:**
1. Run `/status` to confirm SD Card shows OK
2. Verify `memory.json` exists at `/BAN/memory.json` on the SD card
3. Check if the SD card has a physical write-protect switch (slide it to unlock)
4. Check available space with `/storage`

---

### JSON Parse Error on Boot

**Symptoms:** `[MEM] JSON parse error: ...` in Serial Monitor

**Fix:**
1. Eject the SD card and open `memory.json` on your computer
2. Validate it at https://jsonlint.com/
3. Fix any syntax errors (missing commas, unclosed brackets)
4. If unsure, replace the entire file with: `{"memories":[]}`

---

### ESP32 Resets Randomly

**Symptoms:** Spontaneous reboots, "Guru Meditation Error" in serial

**Check:**
1. Run `/status` and check Free Heap — below 20,000 bytes indicates a problem
2. Reduce `MEMORY_JSON_CAPACITY` in `MemoryManager.h` if you have few memories
3. Keep memory record values under 200 characters each
4. Do not store very long strings in memory.json

---

### Bluetooth Disconnects Frequently

**Check:**
1. Make sure the app on your phone does not have an inactivity timeout
2. Keep the phone within 10 meters of the ESP32
3. Reduce 2.4 GHz interference (other routers, microwave ovens, etc.)
4. Note: Enabling Wi-Fi on the ESP32 can reduce Bluetooth range because they share the same antenna

---

### personality.txt Not Loading

**Symptoms:** Serial shows "personality.txt not found on SD card"

**Fix:**
1. Confirm the file path is exactly `/BAN/personality.txt` on the SD card
2. The file must be inside the `BAN` folder, not in the root
3. After placing the file, send `/personality reload` over Bluetooth

---

## 11. Version Roadmap

| Version | Status | Features |
|---------|--------|----------|
| **1.0** | Current | Bluetooth text, SD card, personality, memory CRUD, commands, conversation logging |
| **2.0** | Planned | Wi-Fi, HTTP POST to AI backend, LLM context injection |
| **3.0** | Planned | OLED or LCD display, physical buttons |
| **4.0** | Planned | Microphone input, speaker output, speech recognition |
| **5.0** | Planned | Raspberry Pi local LLM, fully offline AI, semantic memory search |

### Stage Checklist (Version 1.0)

- [x] Stage 1: Boot sequence and SD card initialization
- [x] Stage 2: Bluetooth Classic SPP communication
- [x] Stage 3: Personality and identity loading from SD
- [x] Stage 4: memory.json system with ArduinoJson
- [x] Stage 5: /remember /memory /search /forget /clear_memory commands
- [x] Stage 6: Conversation logging to SD card files
- [ ] Stage 7: Wi-Fi connection and HTTP AI backend
- [ ] Stage 8: Context injection — personality plus memories sent to LLM
- [ ] Stage 9: Async HTTP and response streaming
- [ ] Stage 10: Security hardening and OTA firmware updates

---

## Quick Start Checklist

```
[ ] ESP32 and SD module wired correctly (3.3V, GND, CS, MOSI, MISO, SCK)
[ ] SD card formatted as FAT32
[ ] BAN/ folder copied to SD card root
[ ] PlatformIO or Arduino IDE installed
[ ] ArduinoJson 6.x library installed
[ ] ESP32 Dev Module board selected
[ ] Correct COM port selected
[ ] Firmware compiled successfully (no errors)
[ ] Firmware uploaded successfully
[ ] Serial monitor shows "B.A.N. ONLINE" at 115200 baud
[ ] Bluetooth paired on phone
[ ] Serial Bluetooth Terminal app connected to BAN-Device
[ ] /status command returns correct status
[ ] /memory command lists the 5 default memories
[ ] /remember test message works and saves
[ ] /search test finds the saved message
```

---

*B.A.N. — Raian AI Network — ESP32 v1.0 — 2026-09-25*
