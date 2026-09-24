// ============================================================
// B.A.N. — CommandProcessor.cpp
// Routes messages to commands or AI backend.
// ============================================================

#include "CommandProcessor.h"

CommandProcessor::CommandProcessor()
    : _mem(nullptr), _personality(nullptr),
      _sd(nullptr), _sys(nullptr), _ai(nullptr) {}

void CommandProcessor::begin(MemoryManager*     mem,
                             PersonalityManager* personality,
                             SDManager*          sd,
                             SystemManager*      sys,
                             AIClient*           ai) {
    _mem         = mem;
    _personality = personality;
    _sd          = sd;
    _sys         = sys;
    _ai          = ai;
}

// ─── Main Router ─────────────────────────────────────────────

String CommandProcessor::process(const String& input, BluetoothManager* bt) {
    // Strip protocol prefix (e.g., "USER|Hello" → "Hello")
    String msg = _stripPrefix(input);
    msg.trim();

    if (msg.length() == 0) return "";

    // Log to serial
    Serial.print(F("[CMD] Processing: "));
    Serial.println(msg);

    // ─── Command routing ─────────────────────────────────────
    if (msg.startsWith("/")) {
        // Extract command and arguments
        String lowerMsg = msg;
        lowerMsg.toLowerCase();

        if (lowerMsg == "/help") {
            return _cmdHelp();
        }
        else if (lowerMsg == "/status") {
            return _cmdStatus();
        }
        else if (lowerMsg == "/memory") {
            return _cmdMemory();
        }
        else if (lowerMsg.startsWith("/remember ")) {
            return _cmdRemember(_extractArgs(msg, 10));
        }
        else if (lowerMsg.startsWith("/search ")) {
            return _cmdSearch(_extractArgs(msg, 8));
        }
        else if (lowerMsg.startsWith("/forget ")) {
            return _cmdForget(_extractArgs(msg, 8));
        }
        else if (lowerMsg == "/clear_memory confirm") {
            return _cmdClearMemory("confirm");
        }
        else if (lowerMsg == "/clear_memory") {
            return _cmdClearMemory("");
        }
        else if (lowerMsg == "/personality" || lowerMsg == "/personality show") {
            return _cmdPersonality("show");
        }
        else if (lowerMsg == "/personality reload") {
            return _cmdPersonality("reload");
        }
        else if (lowerMsg == "/reload") {
            return _cmdReload();
        }
        else if (lowerMsg == "/time") {
            return _cmdTime();
        }
        else if (lowerMsg == "/device") {
            return _cmdDevice();
        }
        else if (lowerMsg == "/storage") {
            return _cmdStorage();
        }
        else {
            return "AI|Unknown command: " + msg + "\nType /help for a list of commands.";
        }
    }

    // ─── Non-command: forward to AI or search memory ──────────
    return _forwardToAI(msg);
}

// ─── /help ───────────────────────────────────────────────────

String CommandProcessor::_cmdHelp() {
    String r = "AI|=== B.A.N. COMMANDS ===\n\n";
    r += "/help              - This help list\n";
    r += "/status            - Full system status\n";
    r += "/memory            - List all memories\n";
    r += "/remember <text>   - Save a new memory\n";
    r += "/search <keyword>  - Search memories\n";
    r += "/forget <id>       - Delete memory by ID\n";
    r += "/clear_memory      - Clear all memories\n";
    r += "/personality       - Show personality info\n";
    r += "/personality reload- Reload from SD card\n";
    r += "/reload            - Reload all SD files\n";
    r += "/time              - Show uptime\n";
    r += "/device            - Device info\n";
    r += "/storage           - SD card storage info\n\n";
    r += "Any other text is sent to the AI backend (if available)\n";
    r += "or searched in local memory.";
    return r;
}

// ─── /status ─────────────────────────────────────────────────

String CommandProcessor::_cmdStatus() {
    String r = "AI|=== B.A.N. STATUS ===\n\n";

    r += "ESP32:       ONLINE\n";

    r += "SD Card:     ";
    r += (_sd && _sd->isReady()) ? "OK" : "UNAVAILABLE";
    r += "\n";

    r += "Memory:      ";
    if (_mem && _mem->isAvailable()) {
        r += "OK (";
        r += _mem->getMemoryCount();
        r += " records)";
    } else {
        r += "UNAVAILABLE";
    }
    r += "\n";

    r += "Personality: ";
    r += (_personality && _personality->isLoaded()) ? "LOADED" : "DEFAULT";
    r += "\n";

    r += "AI Backend:  ";
    r += (_ai && _ai->isAvailable()) ? "AVAILABLE" : "UNAVAILABLE";
    r += "\n";

    r += "Free Heap:   ";
    r += ESP.getFreeHeap();
    r += " bytes\n";

    r += "Uptime:      ";
    r += (_sys ? _sys->getUptimeString() : "unknown");
    r += "\n";

    r += "Chip Model:  ";
    r += ESP.getChipModel();
    r += "\n";

    r += "CPU Freq:    ";
    r += ESP.getCpuFreqMHz();
    r += " MHz\n";

    return r;
}

// ─── /memory ─────────────────────────────────────────────────

String CommandProcessor::_cmdMemory() {
    if (!_mem) return "AI|Memory manager not initialized.";

    String content = _mem->getMemorySummary();
    return "AI|" + content;
}

// ─── /remember ───────────────────────────────────────────────

String CommandProcessor::_cmdRemember(const String& args) {
    if (!_mem) return "AI|Memory manager not initialized.";
    if (args.length() == 0) {
        return "AI|Usage: /remember <information to save>\nExample: /remember My RFID project is CHARRMPASS.";
    }

    // Default category is NOTE — the user can expand this
    // In the future, parse "as PROJECT" syntax
    int id = _mem->saveMemory("NOTE", "user_note", args);

    if (id >= 0) {
        String r = "AI|Memory saved. [ID #";
        r += id;
        r += "]\n\"";
        r += args;
        r += "\"";
        return r;
    } else {
        return "AI|ERROR: Failed to save memory. SD card may be unavailable.";
    }
}

// ─── /search ─────────────────────────────────────────────────

String CommandProcessor::_cmdSearch(const String& args) {
    if (!_mem) return "AI|Memory manager not initialized.";
    if (args.length() == 0) {
        return "AI|Usage: /search <keyword>";
    }
    return "AI|" + _mem->searchMemory(args);
}

// ─── /forget ─────────────────────────────────────────────────

String CommandProcessor::_cmdForget(const String& args) {
    if (!_mem) return "AI|Memory manager not initialized.";
    if (args.length() == 0) {
        return "AI|Usage: /forget <memory_id>\nExample: /forget 3";
    }

    int id = args.toInt();
    if (id <= 0) {
        return "AI|Invalid memory ID. Must be a positive integer.";
    }

    if (_mem->deleteMemory(id)) {
        return "AI|Memory #" + String(id) + " deleted.";
    } else {
        return "AI|Memory #" + String(id) + " not found or could not be deleted.";
    }
}

// ─── /clear_memory ───────────────────────────────────────────

String CommandProcessor::_cmdClearMemory(const String& args) {
    if (!_mem) return "AI|Memory manager not initialized.";

    if (args != "confirm") {
        return "AI|WARNING: This will delete ALL stored memories.\n"
               "To confirm, type:\n  /clear_memory confirm";
    }

    if (_mem->clearMemory()) {
        return "AI|All memories have been cleared.";
    } else {
        return "AI|ERROR: Failed to clear memory.";
    }
}

// ─── /personality ────────────────────────────────────────────

String CommandProcessor::_cmdPersonality(const String& args) {
    if (!_personality) return "AI|Personality manager not initialized.";

    if (args == "reload") {
        if (_personality->reload()) {
            return "AI|Personality reloaded from SD card.\nName: " + _personality->getName();
        } else {
            return "AI|Failed to reload personality from SD card.";
        }
    }

    // Show personality
    String r = "AI|=== PERSONALITY ===\n\n";
    r += "Name: " + _personality->getName() + "\n\n";
    r += "--- identity.txt ---\n";
    r += _personality->getIdentity();
    r += "\n--- personality.txt (first 400 chars) ---\n";
    String p = _personality->getPersonality();
    if (p.length() > 400) {
        r += p.substring(0, 400) + "...";
    } else {
        r += p;
    }
    return r;
}

// ─── /reload ─────────────────────────────────────────────────

String CommandProcessor::_cmdReload() {
    String r = "AI|Reloading all SD card files...\n";

    bool memOk = _mem ? _mem->loadMemory() : false;
    bool perOk = _personality ? _personality->reload() : false;

    r += "Memory:      " + String(memOk ? "OK" : "FAILED") + "\n";
    r += "Personality: " + String(perOk ? "OK" : "FAILED") + "\n";

    return r;
}

// ─── /time ───────────────────────────────────────────────────

String CommandProcessor::_cmdTime() {
    return "AI|Uptime: " + (_sys ? _sys->getUptimeString() : "unknown") +
           "\n(No RTC installed — time is relative to boot)";
}

// ─── /device ─────────────────────────────────────────────────

String CommandProcessor::_cmdDevice() {
    String r = "AI|=== DEVICE INFO ===\n\n";
    r += "Name:     B.A.N. (Raian AI Network)\n";
    r += "Version:  BAN-ESP32-1.0\n";
    r += "Chip:     " + String(ESP.getChipModel()) + "\n";
    r += "Revision: " + String(ESP.getChipRevision()) + "\n";
    r += "CPU Freq: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    r += "Flash:    " + String(ESP.getFlashChipSize() / 1024) + " KB\n";
    r += "Free RAM: " + String(ESP.getFreeHeap()) + " bytes\n";
    r += "SDK:      " + String(ESP.getSdkVersion()) + "\n";
    return r;
}

// ─── /storage ────────────────────────────────────────────────

String CommandProcessor::_cmdStorage() {
    String r = "AI|=== STORAGE INFO ===\n\n";

    if (!_sd || !_sd->isReady()) {
        r += "SD Card: UNAVAILABLE\n";
        return r;
    }

    uint64_t total = _sd->totalBytes();
    uint64_t used  = _sd->usedBytes();
    uint64_t free  = total - used;

    r += "SD Card:  MOUNTED\n";
    r += "Total:    " + String((uint32_t)(total / 1024)) + " KB\n";
    r += "Used:     " + String((uint32_t)(used  / 1024)) + " KB\n";
    r += "Free:     " + String((uint32_t)(free  / 1024)) + " KB\n";

    if (_mem) {
        r += "Memories: " + String(_mem->getMemoryCount()) + " records\n";
    }

    return r;
}

// ─── Forward to AI ───────────────────────────────────────────

String CommandProcessor::_forwardToAI(const String& message) {
    if (!_ai || !_ai->isAvailable()) {
        // AI unavailable — search local memory and reply
        String relevant = "";
        if (_mem && _mem->isAvailable()) {
            relevant = _mem->getRelevantMemories(message);
        }

        String r = "AI|I can't reach the AI backend right now.\n";
        r += "Local memory and commands are still available.\n";

        if (relevant.length() > 0) {
            r += "\nRelevant memories:\n";
            r += relevant;
        } else {
            r += "\nTip: Use /search <keyword> to look up stored information.";
        }

        return r;
    }

    // AI is available — build context and send request
    String context = "";
    if (_personality) context = _personality->buildSystemPrompt();
    if (_mem) {
        String memories = _mem->getRelevantMemories(message);
        if (memories.length() > 0) {
            context += "\nMEMORY (relevant):\n" + memories + "\n";
        }
    }

    String response = _ai->sendRequest(context, message);
    return "AI|" + response;
}

// ─── Helpers ─────────────────────────────────────────────────

String CommandProcessor::_extractArgs(const String& input, int cmdLen) {
    if (input.length() <= cmdLen) return "";
    String args = input.substring(cmdLen);
    args.trim();
    return args;
}

String CommandProcessor::_stripPrefix(const String& input) {
    // Remove protocol prefixes: "USER|", "AI|", "SYSTEM|", etc.
    int pipeIdx = input.indexOf('|');
    if (pipeIdx >= 0 && pipeIdx < 10) {
        // Only strip if the prefix is short (not part of content)
        return input.substring(pipeIdx + 1);
    }
    return input;
}
