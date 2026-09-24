// ============================================================
// B.A.N. — SystemManager.h
// Tracks uptime, heap health, and periodic system housekeeping.
// ============================================================
#pragma once

#include <Arduino.h>
#include "../storage/SDManager.h"
#include "../memory/MemoryManager.h"
#include "../personality/PersonalityManager.h"
#include "../bluetooth/BluetoothManager.h"
#include "../ai/AIClient.h"

// How often to run periodic tasks (milliseconds)
#define SYSMANAGER_PERIODIC_MS  30000  // Every 30 seconds

class SystemManager {
public:
    SystemManager();

    void begin();

    // Call from main loop — handles periodic tasks
    void update();

    // Returns uptime as a human-readable string ("5m 23s")
    String getUptimeString();

    // Returns uptime in seconds
    unsigned long getUptimeSeconds();

    // Prints a startup status summary to Serial
    void printStatus(SDManager*          sd,
                     MemoryManager*      mem,
                     PersonalityManager* personality,
                     BluetoothManager*   bt,
                     AIClient*           ai);

private:
    unsigned long _startMs;        // millis() at boot
    unsigned long _lastPeriodicMs; // Last time periodic task ran

    // Periodic: log heap health, etc.
    void _runPeriodic();
};
