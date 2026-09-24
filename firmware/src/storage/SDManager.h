// ============================================================
// B.A.N. — SDManager.h
// Handles microSD card operations using SPI.
// All file paths follow the /BAN/ directory structure.
//
// Pin assignments (adjust to your wiring):
//   CS   → GPIO 5
//   MOSI → GPIO 23
//   MISO → GPIO 19
//   SCK  → GPIO 18
// ============================================================
#pragma once

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

// ─── SD card SPI pin definitions ─────────────────────────────
// Change these to match YOUR wiring if different
#define SD_CS_PIN    5
#define SD_MOSI_PIN  23
#define SD_MISO_PIN  19
#define SD_SCK_PIN   18

// ─── Directory and file path constants ───────────────────────
#define BAN_ROOT_DIR         "/BAN"
#define BAN_KNOWLEDGE_DIR    "/BAN/knowledge"
#define BAN_CONV_DIR         "/BAN/conversations"
#define BAN_LOG_DIR          "/BAN/logs"

#define BAN_PERSONALITY_FILE "/BAN/personality.txt"
#define BAN_IDENTITY_FILE    "/BAN/identity.txt"
#define BAN_SETTINGS_FILE    "/BAN/settings.json"
#define BAN_MEMORY_FILE      "/BAN/memory.json"
#define BAN_USERS_FILE       "/BAN/users.json"
#define BAN_SYSTEM_LOG       "/BAN/logs/system.log"
#define BAN_ERROR_LOG        "/BAN/logs/errors.log"

// Maximum file read size (to prevent OOM)
#define SD_MAX_READ_SIZE     8192

class SDManager {
public:
    SDManager();

    // Mount the SD card — returns true on success
    bool begin();

    // Returns true if SD card is mounted and ready
    bool isReady();

    // Create the /BAN/ directory structure if it doesn't exist
    void ensureDirectoryStructure();

    // ─── File operations ────────────────────────────────────

    // Read entire file into a String. Returns "" on failure.
    String readFile(const char* path);

    // Write (overwrite) a file with given content
    bool writeFile(const char* path, const String& content);

    // Append a line to a file
    bool appendFile(const char* path, const String& line);

    // Check if a file exists
    bool fileExists(const char* path);

    // Delete a file
    bool deleteFile(const char* path);

    // ─── Conversation logging ────────────────────────────────

    // Append a USER/AI exchange to the current conversation file
    void appendConversation(const String& userMsg, const String& aiResponse);

    // Start a new conversation session (creates a new timestamped file)
    void startNewConversation();

    // ─── System logging ─────────────────────────────────────

    // Append a line to the system log
    void logSystem(const String& msg);

    // Append a line to the error log
    void logError(const String& msg);

    // ─── Storage info ────────────────────────────────────────

    // Returns total SD card size in bytes
    uint64_t totalBytes();

    // Returns used bytes on SD card
    uint64_t usedBytes();

private:
    bool    _ready;
    String  _currentConvFile; // Path to current session conversation file

    // Build a timestamped filename for conversations
    String _buildConvFilename();

    // Get a timestamp string (uses millis if no RTC available)
    String _getTimestamp();
};
