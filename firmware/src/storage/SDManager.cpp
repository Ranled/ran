// ============================================================
// B.A.N. — SDManager.cpp
// microSD card file management implementation.
//
// Uses the ESP32 Arduino SD library with SPI interface.
// All paths are under the /BAN/ root directory.
// ============================================================

#include "SDManager.h"

SDManager::SDManager() : _ready(false) {
    _currentConvFile = "";
}

bool SDManager::begin() {
    // Configure SPI pins
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    // Try to mount the SD card
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println(F("[SD] Mount failed. Check wiring and card."));
        _ready = false;
        return false;
    }

    // Check card type
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println(F("[SD] No SD card detected."));
        _ready = false;
        return false;
    }

    _ready = true;

    // Print card info
    Serial.print(F("[SD] Card type: "));
    switch (cardType) {
        case CARD_MMC:  Serial.println(F("MMC")); break;
        case CARD_SD:   Serial.println(F("SDSC")); break;
        case CARD_SDHC: Serial.println(F("SDHC")); break;
        default:        Serial.println(F("UNKNOWN")); break;
    }

    Serial.print(F("[SD] Card size: "));
    Serial.print((uint32_t)(SD.cardSize() / (1024 * 1024)));
    Serial.println(F(" MB"));

    // Start a new conversation session
    startNewConversation();

    return true;
}

bool SDManager::isReady() {
    return _ready;
}

void SDManager::ensureDirectoryStructure() {
    if (!_ready) return;

    // Create top-level directories if they don't exist
    const char* dirs[] = {
        BAN_ROOT_DIR,
        BAN_KNOWLEDGE_DIR,
        BAN_CONV_DIR,
        BAN_LOG_DIR,
        nullptr
    };

    for (int i = 0; dirs[i] != nullptr; i++) {
        if (!SD.exists(dirs[i])) {
            if (SD.mkdir(dirs[i])) {
                Serial.print(F("[SD] Created directory: "));
                Serial.println(dirs[i]);
            } else {
                Serial.print(F("[SD] Failed to create directory: "));
                Serial.println(dirs[i]);
            }
        }
    }

    // Create default files if they don't exist
    if (!SD.exists(BAN_MEMORY_FILE)) {
        writeFile(BAN_MEMORY_FILE, F("{\"memories\":[]}"));
        Serial.println(F("[SD] Created default memory.json"));
    }

    if (!SD.exists(BAN_SETTINGS_FILE)) {
        writeFile(BAN_SETTINGS_FILE,
            F("{\n  \"auto_memory\": false,\n  \"ai_backend\": \"none\",\n  \"language\": \"en\"\n}"));
        Serial.println(F("[SD] Created default settings.json"));
    }
}

// ─── File Operations ─────────────────────────────────────────

String SDManager::readFile(const char* path) {
    if (!_ready) return "";

    File file = SD.open(path, FILE_READ);
    if (!file) {
        Serial.print(F("[SD] Cannot open file: "));
        Serial.println(path);
        return "";
    }

    String content = "";
    size_t bytesRead = 0;

    while (file.available() && bytesRead < SD_MAX_READ_SIZE) {
        content += (char)file.read();
        bytesRead++;
    }

    file.close();
    return content;
}

bool SDManager::writeFile(const char* path, const String& content) {
    if (!_ready) return false;

    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.print(F("[SD] Cannot write file: "));
        Serial.println(path);
        return false;
    }

    file.print(content);
    file.close();
    return true;
}

bool SDManager::appendFile(const char* path, const String& line) {
    if (!_ready) return false;

    File file = SD.open(path, FILE_APPEND);
    if (!file) {
        Serial.print(F("[SD] Cannot append to file: "));
        Serial.println(path);
        return false;
    }

    file.println(line);
    file.close();
    return true;
}

bool SDManager::fileExists(const char* path) {
    if (!_ready) return false;
    return SD.exists(path);
}

bool SDManager::deleteFile(const char* path) {
    if (!_ready) return false;
    return SD.remove(path);
}

// ─── Conversation Logging ─────────────────────────────────────

void SDManager::startNewConversation() {
    if (!_ready) return;
    _currentConvFile = _buildConvFilename();

    // Write session header
    String header = "=== B.A.N. Conversation Session ===\n";
    header += "Started: ";
    header += _getTimestamp();
    header += "\n\n";

    appendFile(_currentConvFile.c_str(), header);
    Serial.print(F("[SD] Conversation file: "));
    Serial.println(_currentConvFile);
}

void SDManager::appendConversation(const String& userMsg, const String& aiResponse) {
    if (!_ready || _currentConvFile.length() == 0) return;

    String entry = "[USER]\n";
    entry += userMsg;
    entry += "\n\n[AI]\n";
    entry += aiResponse;
    entry += "\n\n---\n\n";

    appendFile(_currentConvFile.c_str(), entry);
}

// ─── System Logging ──────────────────────────────────────────

void SDManager::logSystem(const String& msg) {
    if (!_ready) return;
    String line = _getTimestamp() + " [SYS] " + msg;
    appendFile(BAN_SYSTEM_LOG, line);
}

void SDManager::logError(const String& msg) {
    if (!_ready) return;
    String line = _getTimestamp() + " [ERR] " + msg;
    appendFile(BAN_ERROR_LOG, line);
    Serial.print(F("[SD-ERR] "));
    Serial.println(msg);
}

// ─── Storage Info ─────────────────────────────────────────────

uint64_t SDManager::totalBytes() {
    if (!_ready) return 0;
    return SD.totalBytes();
}

uint64_t SDManager::usedBytes() {
    if (!_ready) return 0;
    return SD.usedBytes();
}

// ─── Private Helpers ─────────────────────────────────────────

String SDManager::_buildConvFilename() {
    // Without RTC we use millis as a session ID
    // With RTC this would use actual date/time
    char filename[64];
    unsigned long t = millis();
    // Format: /BAN/conversations/conv_XXXXXXXXXX.txt
    snprintf(filename, sizeof(filename), "%s/conv_%010lu.txt",
             BAN_CONV_DIR, t);
    return String(filename);
}

String SDManager::_getTimestamp() {
    // Simple millis()-based timestamp (seconds since boot)
    // Replace with RTC library calls if an RTC module is added
    unsigned long secs = millis() / 1000;
    char buf[32];
    snprintf(buf, sizeof(buf), "[T+%lus]", secs);
    return String(buf);
}
