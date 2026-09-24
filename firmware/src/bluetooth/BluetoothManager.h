// ============================================================
// B.A.N. — BluetoothManager.h
// Handles ESP32 Bluetooth Classic (SPP) communication.
// Uses non-blocking message framing with a circular buffer.
// ============================================================
#pragma once

#include <Arduino.h>
#include "BluetoothSerial.h"  // ESP32 built-in Bluetooth Serial

// Maximum incoming message length (characters)
#define BT_MAX_MSG_LEN 512

// Message delimiter — marks the end of a complete message
#define BT_MSG_DELIMITER '\n'

class BluetoothManager {
public:
    BluetoothManager();

    // Initialize Bluetooth with the given device name
    // Returns true on success, false on failure
    bool begin(const char* deviceName);

    // Call from main loop — handles incoming bytes non-blocking
    void update();

    // Returns true if a complete message is ready to be read
    bool hasMessage();

    // Returns the next complete message and clears the buffer
    String getMessage();

    // Send a message string over Bluetooth
    // Automatically appends the delimiter
    void sendMessage(const String& msg);

    // Send a raw string without modification
    void sendRaw(const String& raw);

    // Returns true if a phone/device is currently connected
    bool isConnected();

    // Returns the device name used for pairing
    const char* getDeviceName();

    // Disconnect gracefully
    void disconnect();

private:
    BluetoothSerial _btSerial;
    char            _inBuffer[BT_MAX_MSG_LEN + 1]; // Receive buffer
    int             _bufLen;                        // Current buffer fill
    bool            _msgReady;                      // Complete message available
    String          _lastMessage;                   // The ready message
    const char*     _deviceName;
    bool            _initialized;

    // Internal: parse incoming bytes into the buffer
    void _processIncomingBytes();
};
