// ============================================================
// B.A.N. — MemoryManager.h
// Manages the memory.json file on the microSD card.
//
// Memory format (memory.json):
// {
//   "memories": [
//     {
//       "id": 1,
//       "category": "PROJECT",
//       "key": "CHARRMPASS",
//       "value": "RFID vehicle entry and exit monitoring system",
//       "created": "T+Xs"
//     }
//   ]
// }
//
// Categories: PROJECT, PREFERENCE, PERSONAL,
//             KNOWLEDGE, DEVICE, TASK, NOTE, CONVERSATION
// ============================================================
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../storage/SDManager.h"

// Maximum number of memory records we can hold in RAM at once
// ArduinoJson document capacity (adjust if you have many memories)
#define MEMORY_JSON_CAPACITY 8192

// Maximum number of search results to return
#define MEMORY_MAX_RESULTS 5

// Memory record structure
struct MemoryRecord {
    int    id;
    String category;
    String key;
    String value;
    String created;
};

class MemoryManager {
public:
    MemoryManager();

    // Initialize — loads memory.json from SD card
    // Returns true if SD is available and memory loaded
    bool begin(SDManager* sd);

    // ─── CRUD operations ─────────────────────────────────────

    // Save a new memory record. Returns new record ID, or -1 on failure.
    int saveMemory(const String& category, const String& key, const String& value);

    // Load all memory records (reload from disk)
    bool loadMemory();

    // Search memories by keyword (checks key and value fields)
    // Returns a formatted string with matching records
    String searchMemory(const String& keyword);

    // Delete a memory record by ID. Returns true on success.
    bool deleteMemory(int id);

    // Delete ALL memory records. Returns true on success.
    bool clearMemory();

    // Get the number of stored memory records
    int getMemoryCount();

    // Get a formatted summary of all memories (for /memory command)
    String getMemorySummary();

    // Get memories relevant to a message (for AI context building)
    // Returns a compact string of relevant records
    String getRelevantMemories(const String& message);

    // Check if memory system is available
    bool isAvailable();

private:
    SDManager* _sd;
    bool       _available;
    int        _nextId;

    // Persist the full DynamicJsonDocument back to memory.json
    bool _saveToSD(DynamicJsonDocument& doc);

    // Load and parse memory.json, populate doc
    bool _loadFromSD(DynamicJsonDocument& doc);

    // Count records in current memory.json
    int _countRecords(DynamicJsonDocument& doc);

    // Find max ID in current memory.json
    int _findMaxId(DynamicJsonDocument& doc);
};
