// ============================================================
// B.A.N. — BluetoothManager.cpp
// Implementation of Bluetooth Classic SPP communication.
//
// Protocol:
//   Messages are text strings terminated by '\n'.
//   Incoming bytes are buffered until '\n' is received.
//   The complete line is then made available via getMessage().
//
// Message format used by B.A.N.:
//   USER|<text>     — message from user
//   AI|<text>       — response from B.A.N.
//   SYSTEM|<text>   — system notifications
//   MEMORY|<data>   — memory operation result
//   ERROR|<text>    — error report
// ============================================================

#include "BluetoothManager.h"

BluetoothManager::BluetoothManager()
    : _bufLen(0), _msgReady(false), _initialized(false), _deviceName("BAN-Device") {
    memset(_inBuffer, 0, sizeof(_inBuffer));
}

bool BluetoothManager::begin(const char* deviceName) {
    _deviceName = deviceName;

    // Start Bluetooth Serial (Classic SPP)
    if (!_btSerial.begin(deviceName)) {
        Serial.println(F("[BT] Failed to initialize BluetoothSerial."));
        _initialized = false;
        return false;
    }

    _initialized = true;
    Serial.print(F("[BT] Bluetooth started. Device name: "));
    Serial.println(deviceName);
    return true;
}

void BluetoothManager::update() {
    if (!_initialized) return;
    _processIncomingBytes();
}

void BluetoothManager::_processIncomingBytes() {
    // Read all available bytes without blocking
    while (_btSerial.available()) {
        char c = (char)_btSerial.read();

        if (c == BT_MSG_DELIMITER) {
            // End of message — mark as ready
            _inBuffer[_bufLen] = '\0';
            _lastMessage = String(_inBuffer);
            _lastMessage.trim(); // Remove any \r or spaces
            _msgReady = true;
            _bufLen = 0; // Reset buffer for next message
        } else {
            // Accumulate character if within buffer limit
            if (_bufLen < BT_MAX_MSG_LEN) {
                _inBuffer[_bufLen++] = c;
            } else {
                // Buffer overflow — discard current message, reset
                Serial.println(F("[BT] WARNING: Message too long, discarding."));
                _bufLen = 0;
                memset(_inBuffer, 0, sizeof(_inBuffer));
            }
        }
    }
}

bool BluetoothManager::hasMessage() {
    return _msgReady;
}

String BluetoothManager::getMessage() {
    _msgReady = false;
    return _lastMessage;
}

void BluetoothManager::sendMessage(const String& msg) {
    if (!_initialized || !isConnected()) return;
    _btSerial.println(msg); // println adds '\n' delimiter automatically
}

void BluetoothManager::sendRaw(const String& raw) {
    if (!_initialized || !isConnected()) return;
    _btSerial.print(raw);
}

bool BluetoothManager::isConnected() {
    if (!_initialized) return false;
    return _btSerial.connected();
}

const char* BluetoothManager::getDeviceName() {
    return _deviceName;
}

void BluetoothManager::disconnect() {
    if (_initialized) {
        _btSerial.disconnect();
    }
}
