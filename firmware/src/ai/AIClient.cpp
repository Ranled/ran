// ============================================================
// B.A.N. — AIClient.cpp
// Abstract AI backend communication layer.
//
// Version 1.0: AI_BACKEND_NONE (offline only).
// Stage 7-8: Uncomment and implement _sendViaHTTP() when
//            Wi-Fi and the backend server are ready.
// ============================================================

#include "AIClient.h"

// Uncomment these when implementing Wi-Fi HTTP in Stage 7-8:
// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>

AIClient::AIClient()
    : _backendType(AI_BACKEND_NONE), _available(false) {}

void AIClient::begin() {
    // Default to offline mode for Version 1.0
    _backendType = AI_BACKEND_NONE;
    _available   = false;

    Serial.println(F("[AI] AIClient initialized. Backend: NONE (offline mode)"));
    Serial.println(F("[AI] To enable AI: call setBackend() with backend type and endpoint."));
}

void AIClient::update() {
    // Non-blocking: in a future async version this would check
    // for pending responses in an HTTP response queue or BT channel.
    // For Version 1.0, sendRequest() is synchronous, so nothing here.
}

bool AIClient::isAvailable() {
    return _available;
}

void AIClient::setBackend(AIBackendType type, const String& endpoint) {
    _backendType = type;
    _endpoint    = endpoint;

    Serial.print(F("[AI] Backend set to type: "));
    Serial.println((int)type);
    Serial.print(F("[AI] Endpoint: "));
    Serial.println(endpoint);

    // Test if backend is reachable
    _available = _testConnectivity();
    Serial.print(F("[AI] Backend available: "));
    Serial.println(_available ? F("YES") : F("NO"));
}

AIBackendType AIClient::getBackendType() {
    return _backendType;
}

String AIClient::sendRequest(const String& context, const String& message) {
    switch (_backendType) {
        case AI_BACKEND_WIFI:
            return _sendViaHTTP(context, message);

        case AI_BACKEND_NONE:
        default:
            return _sendViaNone(context, message);
    }
}

// ─── Backend: None (Offline) ─────────────────────────────────

String AIClient::_sendViaNone(const String& context, const String& message) {
    // AI is not available — return a helpful local response
    return "AI backend is not configured. "
           "I can still access local memory and commands. "
           "Use /help to see what I can do offline.";
}

// ─── Backend: Wi-Fi HTTP POST ────────────────────────────────
// Uncomment and implement this when Stage 7 (Wi-Fi) is ready.
//
// The expected backend server API:
//   POST /ask
//   Content-Type: application/json
//   Body: { "context": "...", "message": "..." }
//   Response: { "response": "..." }
//
// The backend server (Raspberry Pi / PC) should:
//   1. Receive the request
//   2. Add the API key
//   3. Call the LLM
//   4. Return the response
//   (This keeps API keys off the ESP32)

String AIClient::_sendViaHTTP(const String& context, const String& message) {
    // ── STAGE 7-8 IMPLEMENTATION ──────────────────────────────
    // Uncomment and fill in when Wi-Fi backend is ready:
    //
    // if (WiFi.status() != WL_CONNECTED) {
    //     return "Wi-Fi not connected. Cannot reach AI backend.";
    // }
    //
    // HTTPClient http;
    // http.begin(_endpoint);
    // http.addHeader("Content-Type", "application/json");
    //
    // // Build JSON body
    // DynamicJsonDocument reqDoc(4096);
    // reqDoc["context"] = context;
    // reqDoc["message"] = message;
    // String reqBody;
    // serializeJson(reqDoc, reqBody);
    //
    // int httpCode = http.POST(reqBody);
    //
    // if (httpCode == HTTP_CODE_OK) {
    //     String payload = http.getString();
    //     http.end();
    //
    //     DynamicJsonDocument resDoc(4096);
    //     DeserializationError err = deserializeJson(resDoc, payload);
    //     if (err) return "AI response parse error.";
    //     return resDoc["response"].as<String>();
    // } else {
    //     http.end();
    //     return "HTTP error " + String(httpCode) + " from AI backend.";
    // }

    // Placeholder until Stage 7-8:
    return "Wi-Fi AI backend not yet implemented in this version.";
}

// ─── Connectivity Test ───────────────────────────────────────

bool AIClient::_testConnectivity() {
    if (_backendType == AI_BACKEND_NONE) return false;

    // For Wi-Fi backend: attempt a simple connection test
    // For Version 1.0 this always returns false
    // Implement with HTTPClient.GET() when Stage 7 is active
    return false;
}
