// ============================================================
// B.A.N. — PersonalityManager.cpp
// Loads personality.txt and identity.txt from microSD card.
// ============================================================

#include "PersonalityManager.h"

PersonalityManager::PersonalityManager()
    : _sd(nullptr), _loaded(false) {
    _name = "B.A.N."; // Default name fallback
}

bool PersonalityManager::begin(SDManager* sd) {
    _sd = sd;
    return reload();
}

bool PersonalityManager::reload() {
    if (!_sd || !_sd->isReady()) {
        _loaded = false;
        Serial.println(F("[PERSONALITY] SD not available. Using defaults."));
        return false;
    }

    bool anyLoaded = false;

    // Load personality.txt
    if (_sd->fileExists(BAN_PERSONALITY_FILE)) {
        _personality = _sd->readFile(BAN_PERSONALITY_FILE);
        if (_personality.length() > 0) {
            Serial.println(F("[PERSONALITY] personality.txt loaded."));
            anyLoaded = true;
        }
    } else {
        Serial.println(F("[PERSONALITY] personality.txt not found on SD card."));
        _personality = "Name: B.A.N.\nPersonality: Friendly, direct, technical, helpful.\n";
    }

    // Load identity.txt
    if (_sd->fileExists(BAN_IDENTITY_FILE)) {
        _identity = _sd->readFile(BAN_IDENTITY_FILE);
        if (_identity.length() > 0) {
            Serial.println(F("[PERSONALITY] identity.txt loaded."));
            _name = _parseName(_identity);
            anyLoaded = true;
        }
    } else {
        Serial.println(F("[PERSONALITY] identity.txt not found on SD card."));
        _identity = "Assistant Name: B.A.N.\nVersion: BAN-ESP32-1.0\n";
    }

    _loaded = anyLoaded;
    Serial.print(F("[PERSONALITY] Name: "));
    Serial.println(_name);

    return _loaded;
}

String PersonalityManager::getPersonality() {
    return _personality;
}

String PersonalityManager::getIdentity() {
    return _identity;
}

String PersonalityManager::getName() {
    return _name;
}

bool PersonalityManager::isLoaded() {
    return _loaded;
}

String PersonalityManager::buildSystemPrompt() {
    // Build a compact context string to prepend to AI requests
    String prompt = "SYSTEM:\n";
    prompt += "You are ";
    prompt += _name;
    prompt += ", the Raian AI Network.\n\n";
    prompt += "PERSONALITY:\n";

    // Limit personality to 500 chars to save BT bandwidth
    if (_personality.length() > 500) {
        prompt += _personality.substring(0, 500);
        prompt += "...\n";
    } else {
        prompt += _personality;
    }

    prompt += "\n";
    return prompt;
}

// ─── Private: Parse name from identity.txt ───────────────────

String PersonalityManager::_parseName(const String& content) {
    // Look for "Assistant Name:" line
    int nameIdx = content.indexOf("Assistant Name:");
    if (nameIdx < 0) {
        nameIdx = content.indexOf("Name:");
    }

    if (nameIdx >= 0) {
        int lineEnd = content.indexOf('\n', nameIdx);
        String nameLine;
        if (lineEnd >= 0) {
            nameLine = content.substring(nameIdx, lineEnd);
        } else {
            nameLine = content.substring(nameIdx);
        }

        // Extract value after the colon
        int colonIdx = nameLine.indexOf(':');
        if (colonIdx >= 0) {
            String parsed = nameLine.substring(colonIdx + 1);
            parsed.trim();
            if (parsed.length() > 0) return parsed;
        }
    }

    return "B.A.N."; // Fallback
}
