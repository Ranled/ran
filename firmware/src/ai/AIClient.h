// ============================================================
// B.A.N. — AIClient.h
// Abstract AI communication layer.
//
// This layer sends requests to an external AI backend and
// receives responses. The ESP32 does NOT run a language model.
//
// Supported backends (Version 1.0):
//   - None (offline mode, returns local fallback)
//
// Future backends (add in later stages):
//   - Wi-Fi HTTP POST to a local server or Raspberry Pi
//   - Wi-Fi HTTP POST to a cloud API proxy (never expose keys!)
//   - Secondary Bluetooth channel to a phone AI app
//
// To switch backends:
//   1. Implement the backend in a subclass or swap the
//      _sendViaHTTP() / _sendViaBluetooth() methods.
//   2. No other code needs to change.
// ============================================================
#pragma once

#include <Arduino.h>

// AI backend types
enum AIBackendType {
    AI_BACKEND_NONE    = 0,  // Offline / local only
    AI_BACKEND_WIFI    = 1,  // HTTP POST to local server or cloud proxy
    AI_BACKEND_BT      = 2   // Secondary Bluetooth channel (future)
};

class AIClient {
public:
    AIClient();

    // Initialize the AI client layer
    void begin();

    // Call from main loop — handles async AI response polling
    void update();

    // Returns true if an AI backend is currently reachable
    bool isAvailable();

    // Set the backend type and endpoint
    void setBackend(AIBackendType type, const String& endpoint);

    // Send a request to the AI backend.
    // context: system prompt + relevant memories
    // message: the user's current message
    // Returns the AI-generated response string.
    // Blocks until response received or timeout.
    String sendRequest(const String& context, const String& message);

    // Returns current backend type
    AIBackendType getBackendType();

private:
    AIBackendType _backendType;
    String        _endpoint;   // e.g., "http://192.168.1.100:5000/ask"
    bool          _available;

    // Backend-specific send implementations
    String _sendViaHTTP(const String& context, const String& message);
    String _sendViaNone(const String& context, const String& message);

    // Test connectivity to the backend endpoint
    bool _testConnectivity();
};
