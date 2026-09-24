// ============================================================
// B.A.N. — BLEManager.cpp
// ESP32 BLE GATT Server implementation for Web Bluetooth.
//
// Key design decisions:
//   - Uses Nordic UART-style RX/TX characteristics.
//   - Sends JSON + NULL terminator for reliable framing.
//   - Large messages are chunked at BLE_CHUNK_SIZE bytes.
//   - After disconnect, advertising restarts automatically.
// ============================================================

#include "BLEManager.h"

BLEManager::BLEManager()
    : _server(nullptr), _txChar(nullptr), _rxChar(nullptr),
      _callback(nullptr), _connected(false), _advertising(false),
      _deviceName("BAN-ESP32"), _inBufLen(0) {
    memset(_inBuffer, 0, sizeof(_inBuffer));
}

bool BLEManager::begin(const char* deviceName, BLEMessageCallback callback) {
    _deviceName = deviceName;
    _callback   = callback;

    // Initialize BLE stack
    BLEDevice::init(deviceName);

    // Create GATT server
    _server = BLEDevice::createServer();
    _server->setCallbacks(this);

    // Create B.A.N. service
    BLEService* service = _server->createService(BAN_SERVICE_UUID);

    // ── TX Characteristic (ESP32 → Web App via Notifications) ──
    _txChar = service->createCharacteristic(
        BAN_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    // Add Client Characteristic Configuration Descriptor (required for Notify)
    _txChar->addDescriptor(new BLE2902());

    // ── RX Characteristic (Web App → ESP32 via Write) ──────────
    _rxChar = service->createCharacteristic(
        BAN_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR  // WriteWithoutResponse
    );
    _rxChar->setCallbacks(this);

    // Start the service
    service->start();

    // Start advertising
    _startAdvertising();

    Serial.println(F("[BLE] BLE GATT server started."));
    Serial.print(F("[BLE] Device name: "));
    Serial.println(deviceName);
    Serial.println(F("[BLE] Waiting for Web Bluetooth connection..."));

    return true;
}

void BLEManager::_startAdvertising() {
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(BAN_SERVICE_UUID);
    adv->setScanResponse(true);

    // Set preferred connection interval (min 7.5ms, max 15ms)
    adv->setMinPreferred(0x06);
    adv->setMaxPreferred(0x0C);

    BLEDevice::startAdvertising();
    _advertising = true;

    Serial.println(F("[BLE] Advertising started. Open web app and click Connect."));
}

void BLEManager::update() {
    // Non-blocking — connection state changes are handled by callbacks.
    // This is here for future use (e.g., keep-alive pings, timeout checks).
}

bool BLEManager::sendJSON(const String& json) {
    if (!_connected || !_txChar) {
        Serial.println(F("[BLE] Cannot send: no client connected."));
        return false;
    }

    // Append NULL terminator for framing
    String payload = json + BLE_TERMINATOR;
    const uint8_t* data = (const uint8_t*)payload.c_str();
    size_t totalLen = payload.length();

    // Send in chunks
    size_t offset = 0;
    while (offset < totalLen) {
        size_t chunkLen = min((size_t)BLE_CHUNK_SIZE, totalLen - offset);
        _txChar->setValue(data + offset, chunkLen);
        _txChar->notify();
        offset += chunkLen;

        // Small yield between chunks to prevent WDT timeout
        if (offset < totalLen) {
            delay(10);
        }
    }

    Serial.print(F("[BLE-TX] "));
    // Print only first 100 chars to avoid spamming serial
    if (json.length() > 100) {
        Serial.println(json.substring(0, 100) + "...");
    } else {
        Serial.println(json);
    }

    return true;
}

bool BLEManager::isConnected() {
    return _connected;
}

const char* BLEManager::getDeviceName() {
    return _deviceName;
}

void BLEManager::stop() {
    BLEDevice::stopAdvertising();
    _advertising = false;
}

// ── BLEServerCallbacks ────────────────────────────────────────

void BLEManager::onConnect(BLEServer* server) {
    _connected = true;
    _advertising = false;
    _inBufLen = 0;

    Serial.println(F("[BLE] Client connected!"));
    Serial.print(F("[BLE] Connected clients: "));
    Serial.println(server->getConnectedCount());
}

void BLEManager::onDisconnect(BLEServer* server) {
    _connected = false;
    _inBufLen = 0;

    Serial.println(F("[BLE] Client disconnected."));

    // Restart advertising so the web app can reconnect
    delay(200);
    _startAdvertising();
}

// ── BLECharacteristicCallbacks ────────────────────────────────

void BLEManager::onWrite(BLECharacteristic* characteristic) {
    // This is called when the web app writes to the RX characteristic
    std::string rxValue = characteristic->getValue();

    if (rxValue.length() == 0) return;

    _processIncoming((const uint8_t*)rxValue.data(), rxValue.length());
}

// ── Private: assemble chunks into complete messages ───────────

void BLEManager::_processIncoming(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char c = (char)data[i];

        if (c == BLE_TERMINATOR) {
            // Complete message received
            _inBuffer[_inBufLen] = '\0';
            String msg = String(_inBuffer);
            _inBufLen = 0;

            Serial.print(F("[BLE-RX] "));
            Serial.println(msg.length() > 100 ? msg.substring(0, 100) + "..." : msg);

            // Dispatch to callback
            if (_callback && msg.length() > 0) {
                _callback(msg);
            }

        } else {
            // Accumulate byte
            if (_inBufLen < BLE_MAX_MSG_LEN) {
                _inBuffer[_inBufLen++] = c;
            } else {
                // Buffer overflow — reset and discard
                Serial.println(F("[BLE] ERROR: Incoming message too large. Discarding."));
                _inBufLen = 0;
                memset(_inBuffer, 0, sizeof(_inBuffer));
            }
        }
    }
}
