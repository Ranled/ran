// ============================================================
// B.A.N. — Raian AI Network
// Version: BAN-ESP32-1.0
// File:    main.cpp
// Purpose: Main entry point — initializes all subsystems
//          and runs the event loop.
//
// Architecture:
//   ESP32 = BODY (controller, communication, memory manager)
//   microSD = LONG-TERM MEMORY
//   External AI (phone/PC/Pi) = BRAIN (language model)
//
// Stage 1-6 are implemented in this version:
//   Stage 1: Boot sequence + SD init
//   Stage 2: Bluetooth communication
//   Stage 3: Personality loading
//   Stage 4: Memory JSON system
//   Stage 5: /remember /memory /search /forget commands
//   Stage 6: Conversation logging
// ============================================================

#include <Arduino.h>
#include "bluetooth/BluetoothManager.h"
#include "storage/SDManager.h"
#include "personality/PersonalityManager.h"
#include "memory/MemoryManager.h"
#include "commands/CommandProcessor.h"
#include "system/SystemManager.h"
#include "ai/AIClient.h"

// ─── Module instances ────────────────────────────────────────
BluetoothManager  btManager;
SDManager         sdManager;
PersonalityManager personalityManager;
MemoryManager     memoryManager;
CommandProcessor  cmdProcessor;
SystemManager     sysManager;
AIClient          aiClient;

// ─── Boot sequence ───────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500); // Small delay to allow serial monitor to connect

    // --- Boot banner ---
    Serial.println(F("\n╔══════════════════════════════════════╗"));
    Serial.println(F("║        B.A.N. BOOTING...             ║"));
    Serial.println(F("║   Raian AI Network - ESP32 v1.0      ║"));
    Serial.println(F("╚══════════════════════════════════════╝"));
    Serial.println();

    // --- Initialize System Manager (uptime, free heap tracking) ---
    sysManager.begin();
    Serial.println(F("[SYSTEM] System manager initialized."));

    // --- Initialize SD Card ---
    Serial.print(F("[STORAGE] Initializing microSD..."));
    if (sdManager.begin()) {
        Serial.println(F(" OK"));
        Serial.println(F("[STORAGE] SD Card mounted successfully."));
        sdManager.ensureDirectoryStructure(); // Create /BAN/ folders if missing
    } else {
        Serial.println(F(" FAILED"));
        Serial.println(F("[WARNING] microSD unavailable. Long-term memory is DISABLED."));
        Serial.println(F("[WARNING] B.A.N. will continue with limited functionality."));
    }

    // --- Initialize Personality Manager ---
    Serial.print(F("[PERSONALITY] Loading personality..."));
    if (personalityManager.begin(&sdManager)) {
        Serial.println(F(" OK"));
    } else {
        Serial.println(F(" FAILED (using defaults)"));
    }

    // --- Initialize Memory Manager ---
    Serial.print(F("[MEMORY] Initializing memory system..."));
    if (memoryManager.begin(&sdManager)) {
        Serial.print(F(" OK ("));
        Serial.print(memoryManager.getMemoryCount());
        Serial.println(F(" records loaded)"));
    } else {
        Serial.println(F(" FAILED (memory unavailable)"));
    }

    // --- Initialize AI Client ---
    aiClient.begin();
    Serial.println(F("[AI] AI client layer initialized."));

    // --- Initialize Command Processor ---
    cmdProcessor.begin(&memoryManager, &personalityManager, &sdManager, &sysManager, &aiClient);
    Serial.println(F("[CMD] Command processor ready."));

    // --- Initialize Bluetooth ---
    Serial.print(F("[BLUETOOTH] Starting Bluetooth..."));
    if (btManager.begin("BAN-Device")) {
        Serial.println(F(" OK"));
        Serial.println(F("[BLUETOOTH] Device name: BAN-Device"));
        Serial.println(F("[BLUETOOTH] Waiting for connection..."));
    } else {
        Serial.println(F(" FAILED"));
        Serial.println(F("[ERROR] Bluetooth initialization failed."));
    }

    // --- Boot complete ---
    Serial.println();
    Serial.println(F("╔══════════════════════════════════════╗"));
    Serial.println(F("║          B.A.N. ONLINE               ║"));
    Serial.println(F("╚══════════════════════════════════════╝"));
    Serial.println();

    // Print quick status
    sysManager.printStatus(&sdManager, &memoryManager, &personalityManager, &btManager, &aiClient);
}

// ─── Main event loop ─────────────────────────────────────────
void loop() {
    // Non-blocking loop — all operations use millis()-based timing
    // or event-driven callbacks. No delay() calls in the main loop.

    // 1. Process Bluetooth events (receive/send messages)
    btManager.update();

    // 2. If a complete message was received, process it
    if (btManager.hasMessage()) {
        String incoming = btManager.getMessage();

        Serial.print(F("[BT-IN] "));
        Serial.println(incoming);

        // Route message through command processor
        String response = cmdProcessor.process(incoming, &btManager);

        // Send response back over Bluetooth
        if (response.length() > 0) {
            btManager.sendMessage(response);
            Serial.print(F("[BT-OUT] "));
            Serial.println(response);

            // Log conversation to SD card
            if (sdManager.isReady()) {
                sdManager.appendConversation(incoming, response);
            }
        }
    }

    // 3. System housekeeping (uptime counter, periodic tasks)
    sysManager.update();

    // 4. AI client polling (check for async AI responses)
    aiClient.update();
}
