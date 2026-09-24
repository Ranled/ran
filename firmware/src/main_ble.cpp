// ============================================================
// B.A.N. — main_ble.cpp  (Version 2.0 — BLE + Web HCI)
// Raian AI Network — ESP32 Firmware
//
// This replaces main.cpp for the Web Bluetooth version.
// Rename this file to main.cpp when switching to BLE mode.
//
// Changes from v1.0:
//   - Bluetooth Classic SPP → BLE GATT (Web Bluetooth compatible)
//   - Text protocol → JSON protocol
//   - Commands routed via JSON type field
// ============================================================

#include <Arduino.h>
#include <ArduinoJson.h>

#include "bluetooth/BLEManager.h"
#include "storage/SDManager.h"
#include "personality/PersonalityManager.h"
#include "memory/MemoryManager.h"
#include "system/SystemManager.h"
#include "ai/AIClient.h"

// ─── Module instances ─────────────────────────────────────────
BLEManager          bleManager;
SDManager           sdManager;
PersonalityManager  personalityManager;
MemoryManager       memoryManager;
SystemManager       sysManager;
AIClient            aiClient;

// ─── Forward declarations ─────────────────────────────────────
void onBLEMessage(const String& json);
void handleChatMessage(const String& message);
void handleCommand(const String& command);
void handleMemorySave(JsonObject& doc);
void handleMemoryDelete(int id);
void handleMemorySearch(const String& keyword);
void sendStatus();
void sendMemoryList();
void sendPersonality();
void sendResponse(const String& message);
void sendJSON(const String& type, const String& key, const String& value);
void sendError(const String& code, const String& message);

// ─── Setup ────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("\n╔══════════════════════════════════════╗"));
    Serial.println(F("║    B.A.N. BOOTING (BLE + Web HCI)   ║"));
    Serial.println(F("║   Raian AI Network - ESP32 v2.0      ║"));
    Serial.println(F("╚══════════════════════════════════════╝\n"));

    sysManager.begin();

    // SD Card
    Serial.print(F("[STORAGE] Initializing microSD..."));
    if (sdManager.begin()) {
        Serial.println(F(" OK"));
        sdManager.ensureDirectoryStructure();
    } else {
        Serial.println(F(" FAILED (limited functionality)"));
    }

    // Personality
    Serial.print(F("[PERSONALITY] Loading..."));
    if (personalityManager.begin(&sdManager)) {
        Serial.println(F(" OK"));
    } else {
        Serial.println(F(" FAILED (defaults)"));
    }

    // Memory
    Serial.print(F("[MEMORY] Initializing..."));
    if (memoryManager.begin(&sdManager)) {
        Serial.print(F(" OK ("));
        Serial.print(memoryManager.getMemoryCount());
        Serial.println(F(" records)"));
    } else {
        Serial.println(F(" FAILED"));
    }

    // AI Client
    aiClient.begin();

    // BLE — pass the message callback
    Serial.print(F("[BLE] Starting BLE GATT server..."));
    if (bleManager.begin("BAN-ESP32", onBLEMessage)) {
        Serial.println(F(" OK"));
    } else {
        Serial.println(F(" FAILED"));
    }

    Serial.println(F("\n╔══════════════════════════════════════╗"));
    Serial.println(F("║          B.A.N. ONLINE               ║"));
    Serial.println(F("╚══════════════════════════════════════╝"));
}

// ─── Main loop ────────────────────────────────────────────────
void loop() {
    bleManager.update();
    sysManager.update();
    aiClient.update();
}

// ─── BLE Message Dispatcher ───────────────────────────────────
// Called by BLEManager when a complete JSON message arrives.

void onBLEMessage(const String& json) {
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, json);

    if (err) {
        Serial.print(F("[PROTOCOL] JSON parse error: "));
        Serial.println(err.c_str());
        sendError("PARSE_ERROR", "Invalid JSON received.");
        return;
    }

    String type = doc["type"].as<String>();

    if (type == "chat") {
        handleChatMessage(doc["message"].as<String>());

    } else if (type == "command") {
        handleCommand(doc["command"].as<String>());

    } else if (type == "memory_save") {
        JsonObject obj = doc.as<JsonObject>();
        handleMemorySave(obj);

    } else if (type == "memory_delete") {
        handleMemoryDelete(doc["id"].as<int>());

    } else if (type == "memory_search") {
        handleMemorySearch(doc["keyword"].as<String>());

    } else {
        sendError("UNKNOWN_TYPE", "Unknown message type: " + type);
    }
}

// ─── Chat message handler ─────────────────────────────────────

void handleChatMessage(const String& message) {
    if (message.length() == 0) return;

    // Check if it's a slash command
    if (message.startsWith("/")) {
        // Handle common commands inline
        String lower = message;
        lower.toLowerCase();

        if (lower == "/status") {
            sendStatus();
            return;
        }
        if (lower == "/memory") {
            sendMemoryList();
            return;
        }
        if (lower == "/personality" || lower == "/personality show") {
            sendPersonality();
            return;
        }
        if (lower == "/personality reload") {
            personalityManager.reload();
            sendResponse("Personality reloaded from SD card.");
            return;
        }
        if (lower.startsWith("/remember ")) {
            String info = message.substring(10);
            info.trim();
            int id = memoryManager.saveMemory("NOTE", "user_note", info);
            if (id >= 0) {
                // Confirm via memory_saved message
                DynamicJsonDocument resp(128);
                resp["type"] = "memory_saved";
                resp["id"] = id;
                String out;
                serializeJson(resp, out);
                bleManager.sendJSON(out);
            } else {
                sendError("MEMORY_SAVE_FAILED", "Could not save memory. SD may be unavailable.");
            }
            return;
        }
        if (lower.startsWith("/search ")) {
            String kw = message.substring(8);
            kw.trim();
            String result = memoryManager.searchMemory(kw);
            sendResponse(result);
            return;
        }
        if (lower.startsWith("/forget ")) {
            int id = message.substring(8).toInt();
            if (memoryManager.deleteMemory(id)) {
                DynamicJsonDocument resp(64);
                resp["type"] = "memory_deleted";
                resp["id"] = id;
                String out;
                serializeJson(resp, out);
                bleManager.sendJSON(out);
            } else {
                sendError("MEMORY_DELETE_FAILED", "Memory #" + String(id) + " not found.");
            }
            return;
        }
    }

    // Not a command — forward to AI backend or respond locally
    String context = "";
    if (personalityManager.isLoaded()) {
        context = personalityManager.buildSystemPrompt();
    }

    String relevant = memoryManager.getRelevantMemories(message);
    if (relevant.length() > 0) {
        context += "\nMEMORY:\n" + relevant;
    }

    String response;
    if (aiClient.isAvailable()) {
        response = aiClient.sendRequest(context, message);
    } else {
        // Offline response
        response = "AI backend is unavailable. Local memory and commands are still active.";
        if (relevant.length() > 0) {
            response += "\n\nRelevant memory:\n" + relevant;
        }
    }

    sendResponse(response);

    // Log conversation
    if (sdManager.isReady()) {
        sdManager.appendConversation(message, response);
    }
}

// ─── Command handler ──────────────────────────────────────────

void handleCommand(const String& command) {
    if (command == "GET_STATUS") {
        sendStatus();
    } else if (command == "GET_PERSONALITY") {
        sendPersonality();
    } else if (command == "GET_MEMORY") {
        sendMemoryList();
    } else if (command == "RELOAD_PERSONALITY") {
        personalityManager.reload();
        sendPersonality();
    } else if (command == "GET_CONVERSATION") {
        sendResponse("Conversation retrieval not yet implemented.");
    } else if (command == "CLEAR_CONVERSATION") {
        sdManager.startNewConversation();
        sendResponse("Conversation cleared.");
    } else {
        sendError("UNKNOWN_COMMAND", "Unknown command: " + command);
    }
}

// ─── Memory actions ───────────────────────────────────────────

void handleMemorySave(JsonObject& doc) {
    String category = doc["category"].as<String>();
    String key      = doc["key"].as<String>();
    String value    = doc["value"].as<String>();

    if (key.length() == 0 || value.length() == 0) {
        sendError("MEMORY_INVALID", "Key and value are required.");
        return;
    }

    category.toUpperCase();
    int id = memoryManager.saveMemory(category, key, value);

    if (id >= 0) {
        DynamicJsonDocument resp(128);
        resp["type"] = "memory_saved";
        resp["id"] = id;
        String out;
        serializeJson(resp, out);
        bleManager.sendJSON(out);
    } else {
        sendError("MEMORY_SAVE_FAILED", "Could not save memory. SD may be unavailable.");
    }
}

void handleMemoryDelete(int id) {
    if (memoryManager.deleteMemory(id)) {
        DynamicJsonDocument resp(64);
        resp["type"] = "memory_deleted";
        resp["id"] = id;
        String out;
        serializeJson(resp, out);
        bleManager.sendJSON(out);
    } else {
        sendError("MEMORY_DELETE_FAILED", "Memory #" + String(id) + " not found.");
    }
}

void handleMemorySearch(const String& keyword) {
    String result = memoryManager.searchMemory(keyword);
    sendResponse(result);
}

// ─── Send helpers ─────────────────────────────────────────────

void sendResponse(const String& message) {
    DynamicJsonDocument doc(1024);
    doc["type"]    = "response";
    doc["message"] = message;
    String out;
    serializeJson(doc, out);
    bleManager.sendJSON(out);
}

void sendStatus() {
    DynamicJsonDocument doc(512);
    doc["type"]        = "status";
    doc["esp32"]       = true;
    doc["bluetooth"]   = bleManager.isConnected();
    doc["wifi"]        = false; // Will be true in Stage 7
    doc["sd"]          = sdManager.isReady();
    doc["ai"]          = aiClient.isAvailable();
    doc["memory"]      = memoryManager.isAvailable();
    doc["freeHeap"]    = (int)ESP.getFreeHeap();
    doc["uptime"]      = sysManager.getUptimeString();
    doc["firmware"]    = "BAN-ESP32-2.0";
    doc["memoryCount"] = memoryManager.getMemoryCount();
    String out;
    serializeJson(doc, out);
    bleManager.sendJSON(out);
}

void sendMemoryList() {
    // Read current memory.json and send as memory_list
    String json = sdManager.readFile(BAN_MEMORY_FILE);

    if (json.length() == 0) {
        // Send empty list
        bleManager.sendJSON("{\"type\":\"memory_list\",\"memories\":[]}");
        return;
    }

    // Wrap the existing memories array in a memory_list message
    // We do this by building the response manually to avoid double parsing
    DynamicJsonDocument src(8192);
    DeserializationError err = deserializeJson(src, json);

    if (err) {
        sendError("MEMORY_PARSE_ERROR", "Could not read memory.json.");
        return;
    }

    DynamicJsonDocument resp(8192);
    resp["type"] = "memory_list";
    resp["memories"] = src["memories"];

    String out;
    serializeJson(resp, out);
    bleManager.sendJSON(out);
}

void sendPersonality() {
    DynamicJsonDocument doc(2048);
    doc["type"]    = "personality";
    doc["name"]    = personalityManager.getName();
    doc["content"] = personalityManager.getPersonality();
    String out;
    serializeJson(doc, out);
    bleManager.sendJSON(out);
}

void sendError(const String& code, const String& message) {
    DynamicJsonDocument doc(256);
    doc["type"]    = "error";
    doc["code"]    = code;
    doc["message"] = message;
    String out;
    serializeJson(doc, out);
    bleManager.sendJSON(out);
}
