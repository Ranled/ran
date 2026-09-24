// ============================================================
// B.A.N. — BLEManager.h
// ESP32 BLE GATT Server for Web Bluetooth communication.
//
// This replaces BluetoothManager (Classic SPP) from v1.0.
// Uses BLE because Web Bluetooth API only supports BLE, NOT
// Bluetooth Classic SPP.
//
// UUIDs (must match the web app exactly):
//   Service:  4fafc201-1fb5-459e-8fcc-c5c9c3319141
//   TX char:  beb5483e-36e1-4688-b7f5-ea07361b26a8  (Notify)
//   RX char:  6e400002-b5a3-f393-e0a9-e50e24dcca9e  (Write)
//
// Protocol:
//   JSON messages terminated by 0x00 (NULL byte).
//   Messages larger than MTU are sent in chunks.
// ============================================================
#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ── UUIDs (must match web/src/services/bluetooth.ts) ─────────
#define BAN_SERVICE_UUID  "4fafc201-1fb5-459e-8fcc-c5c9c3319141"
#define BAN_TX_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BAN_RX_UUID       "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

// Maximum chunk size per BLE notification (bytes)
#define BLE_CHUNK_SIZE    512

// Maximum incoming message buffer
#define BLE_MAX_MSG_LEN   2048

// Message terminator (matches web app)
#define BLE_TERMINATOR    '\x00'

// Callback for when a complete JSON message arrives from the web
typedef void (*BLEMessageCallback)(const String& jsonMessage);

class BLEManager : public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
    BLEManager();

    // Initialize BLE and start advertising
    // deviceName: what appears in the browser's Bluetooth picker
    // callback:   called when a complete message arrives from web
    bool begin(const char* deviceName, BLEMessageCallback callback);

    // Call from main loop — non-blocking
    void update();

    // Send a JSON string to the web app (chunked automatically)
    // Automatically appends the NULL terminator
    bool sendJSON(const String& json);

    // Returns true if a web browser is currently connected
    bool isConnected();

    // Returns the current device name
    const char* getDeviceName();

    // Gracefully stop advertising and disconnect
    void stop();

    // BLE server callbacks (called by BLE library)
    void onConnect(BLEServer* server) override;
    void onDisconnect(BLEServer* server) override;

    // BLE characteristic callbacks
    void onWrite(BLECharacteristic* characteristic) override;

private:
    BLEServer*         _server;
    BLECharacteristic* _txChar;  // Notify → Web
    BLECharacteristic* _rxChar;  // Write  ← Web
    BLEMessageCallback _callback;

    bool   _connected;
    bool   _advertising;
    const char* _deviceName;

    // Buffer for assembling chunked incoming messages
    char   _inBuffer[BLE_MAX_MSG_LEN + 1];
    int    _inBufLen;

    // Start advertising so browsers can discover the device
    void _startAdvertising();

    // Process incoming bytes from the RX characteristic
    void _processIncoming(const uint8_t* data, size_t len);
};
