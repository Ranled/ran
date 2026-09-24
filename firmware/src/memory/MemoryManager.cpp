// ============================================================
// B.A.N. — MemoryManager.cpp
// Persistent memory system backed by memory.json on microSD.
//
// All operations directly read/write the JSON file on the SD card
// to avoid large RAM usage. For short memory lists this is fine.
// For hundreds of records, consider an indexed approach.
// ============================================================

#include "MemoryManager.h"

MemoryManager::MemoryManager()
    : _sd(nullptr), _available(false), _nextId(1) {}

bool MemoryManager::begin(SDManager* sd) {
    _sd = sd;

    if (!_sd || !_sd->isReady()) {
        _available = false;
        return false;
    }

    _available = true;

    // Calculate what the next ID should be
    DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
    if (_loadFromSD(doc)) {
        _nextId = _findMaxId(doc) + 1;
    } else {
        _nextId = 1;
    }

    return true;
}

bool MemoryManager::isAvailable() {
    return _available && _sd && _sd->isReady();
}

// ─── Save Memory ─────────────────────────────────────────────

int MemoryManager::saveMemory(const String& category, const String& key, const String& value) {
    if (!isAvailable()) return -1;

    DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);

    // Load existing memories
    _loadFromSD(doc);

    // Ensure "memories" array exists
    if (!doc.containsKey("memories")) {
        doc.createNestedArray("memories");
    }

    JsonArray memories = doc["memories"];

    // Create new record
    JsonObject newRecord = memories.createNestedObject();
    newRecord["id"]       = _nextId;
    newRecord["category"] = category;
    newRecord["key"]      = key;
    newRecord["value"]    = value;

    // Timestamp (millis-based; replace with RTC date if available)
    char ts[32];
    snprintf(ts, sizeof(ts), "T+%lus", millis() / 1000);
    newRecord["created"]  = ts;

    int assignedId = _nextId;
    _nextId++;

    // Save back to SD
    if (_saveToSD(doc)) {
        Serial.print(F("[MEM] Memory saved. ID="));
        Serial.println(assignedId);
        return assignedId;
    } else {
        Serial.println(F("[MEM] Failed to save memory to SD."));
        return -1;
    }
}

// ─── Load Memory ─────────────────────────────────────────────

bool MemoryManager::loadMemory() {
    if (!isAvailable()) return false;

    DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
    bool ok = _loadFromSD(doc);

    if (ok) {
        _nextId = _findMaxId(doc) + 1;
    }

    return ok;
}

// ─── Get Memory Count ────────────────────────────────────────

int MemoryManager::getMemoryCount() {
    if (!isAvailable()) return 0;

    DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
    if (!_loadFromSD(doc)) return 0;
    return _countRecords(doc);
}

// ─── Get Memory Summary ──────────────────────────────────────

String MemoryManager::getMemorySummary() {
    if (!isAvailable()) {
        return F("Memory system unavailable (SD card not ready).");
    }

    DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
    if (!_loadFromSD(doc)) {
        return F("No memories stored yet.");
    }

    int count = _countRecords(doc);
    if (count == 0) {
        return F("No memories stored yet.");
    }

    String result = "Memory system contains ";
    result += count;
    result += " record(s):\n\n";

    JsonArray memories = doc["memories"];
    for (JsonObject mem : memories) {
        result += "[#";
        result += mem["id"].as<int>();
        result += "] [";
        result += mem["category"].as<String>();
        result += "] ";
        result += mem["key"].as<String>();
        result += ": ";
        result += mem["value"].as<String>();
        result += "\n";
    }

    return result;
}

// ─── Search Memory ───────────────────────────────────────────

String MemoryManager::searchMemory(const String& keyword) {
    if (!isAvailable()) {
        return F("Memory system unavailable.");
    }

    if (keyword.length() == 0) {
        return F("Please provide a search keyword.");
    }

    DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
    if (!_loadFromSD(doc)) {
        return F("No memories stored.");
    }

    String lowerKeyword = keyword;
    lowerKeyword.toLowerCase();

    String results = "";
    int found = 0;

    JsonArray memories = doc["memories"];
    for (JsonObject mem : memories) {
        if (found >= MEMORY_MAX_RESULTS) break;

        String keyStr   = mem["key"].as<String>();
        String valStr   = mem["value"].as<String>();
        String catStr   = mem["category"].as<String>();

        String lk = keyStr;   lk.toLowerCase();
        String lv = valStr;   lv.toLowerCase();
        String lc = catStr;   lc.toLowerCase();

        // Check if keyword appears in key, value, or category
        if (lk.indexOf(lowerKeyword) >= 0 ||
            lv.indexOf(lowerKeyword) >= 0 ||
            lc.indexOf(lowerKeyword) >= 0) {

            results += "[#";
            results += mem["id"].as<int>();
            results += "] [";
            results += catStr;
            results += "] ";
            results += keyStr;
            results += ": ";
            results += valStr;
            results += "\n";
            found++;
        }
    }

    if (found == 0) {
        return "No memories found matching: " + keyword;
    }

    String header = "Found ";
    header += found;
    header += " result(s) for \"";
    header += keyword;
    header += "\":\n\n";
    return header + results;
}

// ─── Get Relevant Memories (for AI context) ──────────────────

String MemoryManager::getRelevantMemories(const String& message) {
    if (!isAvailable()) return "";

    DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
    if (!_loadFromSD(doc)) return "";

    String lowerMsg = message;
    lowerMsg.toLowerCase();

    String relevant = "";
    int found = 0;

    JsonArray memories = doc["memories"];
    for (JsonObject mem : memories) {
        if (found >= MEMORY_MAX_RESULTS) break;

        String keyStr = mem["key"].as<String>();
        String valStr = mem["value"].as<String>();
        String lk = keyStr; lk.toLowerCase();
        String lv = valStr; lv.toLowerCase();

        // Check if any word from key/value appears in the message
        if (lowerMsg.indexOf(lk) >= 0 || lowerMsg.indexOf(lv) >= 0) {
            relevant += "[";
            relevant += mem["category"].as<String>();
            relevant += "] ";
            relevant += keyStr;
            relevant += ": ";
            relevant += valStr;
            relevant += "\n";
            found++;
        }
    }

    return relevant;
}

// ─── Delete Memory ───────────────────────────────────────────

bool MemoryManager::deleteMemory(int id) {
    if (!isAvailable()) return false;

    DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
    if (!_loadFromSD(doc)) return false;

    JsonArray memories = doc["memories"];
    bool found = false;

    // We need to rebuild the array without the target ID
    DynamicJsonDocument newDoc(MEMORY_JSON_CAPACITY);
    JsonArray newMemories = newDoc.createNestedArray("memories");

    for (JsonObject mem : memories) {
        if (mem["id"].as<int>() == id) {
            found = true; // Skip this record (delete it)
        } else {
            // Copy record to new document
            JsonObject copy = newMemories.createNestedObject();
            copy["id"]       = mem["id"];
            copy["category"] = mem["category"];
            copy["key"]      = mem["key"];
            copy["value"]    = mem["value"];
            copy["created"]  = mem["created"];
        }
    }

    if (!found) {
        Serial.print(F("[MEM] Memory ID not found: "));
        Serial.println(id);
        return false;
    }

    return _saveToSD(newDoc);
}

// ─── Clear All Memory ────────────────────────────────────────

bool MemoryManager::clearMemory() {
    if (!isAvailable()) return false;

    DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
    doc.createNestedArray("memories");
    _nextId = 1;

    return _saveToSD(doc);
}

// ─── Private: Load from SD ───────────────────────────────────

bool MemoryManager::_loadFromSD(DynamicJsonDocument& doc) {
    String json = _sd->readFile(BAN_MEMORY_FILE);

    if (json.length() == 0) {
        // Empty or missing file — start fresh
        doc.createNestedArray("memories");
        return false;
    }

    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.print(F("[MEM] JSON parse error: "));
        Serial.println(err.c_str());
        doc.createNestedArray("memories");
        return false;
    }

    return true;
}

// ─── Private: Save to SD ─────────────────────────────────────

bool MemoryManager::_saveToSD(DynamicJsonDocument& doc) {
    String json;
    serializeJsonPretty(doc, json);
    return _sd->writeFile(BAN_MEMORY_FILE, json);
}

// ─── Private: Count Records ──────────────────────────────────

int MemoryManager::_countRecords(DynamicJsonDocument& doc) {
    if (!doc.containsKey("memories")) return 0;
    return doc["memories"].size();
}

// ─── Private: Find Max ID ────────────────────────────────────

int MemoryManager::_findMaxId(DynamicJsonDocument& doc) {
    if (!doc.containsKey("memories")) return 0;

    int maxId = 0;
    JsonArray memories = doc["memories"];
    for (JsonObject mem : memories) {
        int id = mem["id"].as<int>();
        if (id > maxId) maxId = id;
    }
    return maxId;
}
