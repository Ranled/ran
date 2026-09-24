// ============================================================
// B.A.N. — CommandProcessor.h
// Parses incoming messages and routes them to the correct handler.
//
// Commands are identified by the leading '/' character.
// Non-command messages are routed to the AI backend (if available)
// or answered locally using memory search.
//
// Supported commands:
//   /help            — show command list
//   /status          — full system status
//   /memory          — list stored memories
//   /remember <text> — save a new memory
//   /search <kw>     — search memories by keyword
//   /forget <id>     — delete memory by ID
//   /clear_memory    — clear all memory (with confirmation)
//   /clear_memory confirm — confirmed clear
//   /personality     — show personality summary
//   /personality show — same as above
//   /personality reload — reload from SD card
//   /reload          — reload all files from SD
//   /time            — uptime info
//   /device          — device hardware info
//   /storage         — SD card storage info
// ============================================================
#pragma once

#include <Arduino.h>
#include "../memory/MemoryManager.h"
#include "../personality/PersonalityManager.h"
#include "../storage/SDManager.h"
#include "../system/SystemManager.h"
#include "../ai/AIClient.h"
#include "../bluetooth/BluetoothManager.h"

class CommandProcessor {
public:
    CommandProcessor();

    // Initialize with pointers to all subsystems
    void begin(MemoryManager*     mem,
               PersonalityManager* personality,
               SDManager*          sd,
               SystemManager*      sys,
               AIClient*           ai);

    // Process an incoming message string.
    // Returns the response string to send back.
    // Pass btManager for sending multi-line responses.
    String process(const String& input, BluetoothManager* bt);

private:
    MemoryManager*     _mem;
    PersonalityManager* _personality;
    SDManager*          _sd;
    SystemManager*      _sys;
    AIClient*           _ai;

    // ─── Command handlers ────────────────────────────────────
    String _cmdHelp();
    String _cmdStatus();
    String _cmdMemory();
    String _cmdRemember(const String& args);
    String _cmdSearch(const String& args);
    String _cmdForget(const String& args);
    String _cmdClearMemory(const String& args);
    String _cmdPersonality(const String& args);
    String _cmdReload();
    String _cmdTime();
    String _cmdDevice();
    String _cmdStorage();

    // Forward non-command text to AI backend
    String _forwardToAI(const String& message);

    // Extract arguments after a command word
    // e.g., for "/remember Hello world", returns "Hello world"
    String _extractArgs(const String& input, int cmdLen);

    // Strip the protocol prefix (e.g., "USER|") from input
    String _stripPrefix(const String& input);
};
