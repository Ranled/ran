// ============================================================
// B.A.N. — PersonalityManager.h
// Loads and manages the personality and identity from SD card.
//
// Files read:
//   /BAN/personality.txt  — personality traits and style
//   /BAN/identity.txt     — name, version, purpose
//
// These files can be edited on the SD card without recompiling.
// ============================================================
#pragma once

#include <Arduino.h>
#include "../storage/SDManager.h"

class PersonalityManager {
public:
    PersonalityManager();

    // Load personality and identity from SD card
    // Returns true if at least one file loaded successfully
    bool begin(SDManager* sd);

    // Reload files from SD card (e.g., after editing on PC)
    bool reload();

    // Returns the full personality text
    String getPersonality();

    // Returns the full identity text
    String getIdentity();

    // Returns the assistant's name (parsed from identity.txt)
    String getName();

    // Returns true if personality is loaded
    bool isLoaded();

    // Returns a compact system prompt for AI context injection
    // (personality + identity, trimmed for size)
    String buildSystemPrompt();

private:
    SDManager* _sd;
    String     _personality;
    String     _identity;
    String     _name;
    bool       _loaded;

    // Parse the name from identity.txt content
    String _parseName(const String& identityContent);
};
