// ============================================================
// B.A.N. — SystemManager.cpp
// ============================================================

#include "SystemManager.h"

SystemManager::SystemManager()
    : _startMs(0), _lastPeriodicMs(0) {}

void SystemManager::begin() {
    _startMs        = millis();
    _lastPeriodicMs = millis();
}

void SystemManager::update() {
    unsigned long now = millis();

    // Run periodic tasks (non-blocking check using millis)
    if (now - _lastPeriodicMs >= SYSMANAGER_PERIODIC_MS) {
        _lastPeriodicMs = now;
        _runPeriodic();
    }
}

void SystemManager::_runPeriodic() {
    // Log heap status to serial for debugging
    Serial.print(F("[SYS] Uptime: "));
    Serial.print(getUptimeString());
    Serial.print(F("  Free Heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
}

unsigned long SystemManager::getUptimeSeconds() {
    return (millis() - _startMs) / 1000;
}

String SystemManager::getUptimeString() {
    unsigned long secs = getUptimeSeconds();
    unsigned long mins = secs / 60;
    unsigned long hrs  = mins / 60;
    secs %= 60;
    mins %= 60;

    char buf[32];
    if (hrs > 0) {
        snprintf(buf, sizeof(buf), "%luh %lum %lus", hrs, mins, secs);
    } else if (mins > 0) {
        snprintf(buf, sizeof(buf), "%lum %lus", mins, secs);
    } else {
        snprintf(buf, sizeof(buf), "%lus", secs);
    }
    return String(buf);
}

void SystemManager::printStatus(SDManager*          sd,
                                MemoryManager*      mem,
                                PersonalityManager* personality,
                                BluetoothManager*   bt,
                                AIClient*           ai) {
    Serial.println(F("─── B.A.N. STATUS ──────────────────────"));
    Serial.print(F("  SD Card:     "));
    Serial.println((sd && sd->isReady())                ? F("OK")          : F("UNAVAILABLE"));
    Serial.print(F("  Memory:      "));
    Serial.println((mem && mem->isAvailable())           ? F("OK")          : F("UNAVAILABLE"));
    Serial.print(F("  Personality: "));
    Serial.println((personality && personality->isLoaded()) ? F("LOADED")   : F("DEFAULT"));
    Serial.print(F("  AI Backend:  "));
    Serial.println((ai && ai->isAvailable())             ? F("AVAILABLE")  : F("UNAVAILABLE"));
    Serial.print(F("  Bluetooth:   "));
    Serial.println(bt->isConnected()                     ? F("CONNECTED")  : F("WAITING"));
    Serial.print(F("  Free Heap:   "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    Serial.println(F("────────────────────────────────────────"));
}
