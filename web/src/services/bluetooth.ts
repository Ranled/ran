// ============================================================
// B.A.N. — services/bluetooth.ts
// Web Bluetooth API interface for BLE communication with ESP32
//
// B.A.N. BLE UUIDs (must match ESP32 firmware exactly):
//
//   Service:         4fafc201-1fb5-459e-8fcc-c5c9c3319141
//   TX (ESP32→Web):  beb5483e-36e1-4688-b7f5-ea07361b26a8
//   RX (Web→ESP32):  6e400002-b5a3-f393-e0a9-e50e24dcca9e
//
// Protocol:
//   Messages are JSON strings, chunked into 512-byte packets.
//   Multi-chunk messages are delimited by \x00 (NULL byte) at end.
// ============================================================

import type { OutboundMessage, InboundMessage } from '../types/protocol';

// ── BLE UUIDs ────────────────────────────────────────────────
export const BAN_SERVICE_UUID  = '4fafc201-1fb5-459e-8fcc-c5c9c3319141';
export const BAN_TX_UUID       = 'beb5483e-36e1-4688-b7f5-ea07361b26a8'; // Notify
export const BAN_RX_UUID       = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // Write

// Max BLE payload per packet (ESP32 default ATT MTU minus overhead)
const BLE_CHUNK_SIZE = 512;

// Message terminator
const MSG_TERMINATOR = '\x00';

export type MessageHandler = (msg: InboundMessage) => void;
export type ConnectionHandler = (connected: boolean) => void;
export type ErrorHandler = (error: string) => void;

class BLEBluetoothService {
  private device: BluetoothDevice | null = null;
  private server: BluetoothRemoteGATTServer | null = null;
  private txCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private rxCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

  private messageHandlers: MessageHandler[] = [];
  private connectionHandlers: ConnectionHandler[] = [];
  private errorHandlers: ErrorHandler[] = [];

  // Buffer for assembling chunked incoming messages
  private receiveBuffer = '';

  // ── Public API ──────────────────────────────────────────────

  /** Check if Web Bluetooth is available in this browser */
  isSupported(): boolean {
    return typeof navigator !== 'undefined' &&
           'bluetooth' in navigator;
  }

  /** Returns true if currently connected to the ESP32 */
  isConnected(): boolean {
    return this.server?.connected ?? false;
  }

  /** Connect to the B.A.N. ESP32 device */
  async connect(): Promise<void> {
    if (!this.isSupported()) {
      throw new Error('Web Bluetooth is not supported in this browser.');
    }

    try {
      // Show browser's Bluetooth device picker
      this.device = await navigator.bluetooth.requestDevice({
        filters: [
          { name: 'RAN-ESP32' },
          { namePrefix: 'RAN' },
          { name: 'BAN-ESP32' },
          { namePrefix: 'BAN' }
        ],
        optionalServices: [BAN_SERVICE_UUID]
      });

      // Listen for unexpected disconnections
      this.device.addEventListener('gattserverdisconnected', () => {
        this._handleDisconnect();
      });

      // Connect to the GATT server
      this.server = await this.device.gatt!.connect();

      // Get the B.A.N. service
      const service = await this.server.getPrimaryService(BAN_SERVICE_UUID);

      // Get TX characteristic (ESP32 → Web): subscribe to notifications
      this.txCharacteristic = await service.getCharacteristic(BAN_TX_UUID);
      await this.txCharacteristic.startNotifications();
      this.txCharacteristic.addEventListener(
        'characteristicvaluechanged',
        this._handleIncoming.bind(this)
      );

      // Get RX characteristic (Web → ESP32): write
      this.rxCharacteristic = await service.getCharacteristic(BAN_RX_UUID);

      this._notifyConnection(true);

    } catch (err) {
      const msg = err instanceof Error ? err.message : String(err);
      this._notifyError(`Connection failed: ${msg}`);
      throw err;
    }
  }

  /** Disconnect from the ESP32 */
  async disconnect(): Promise<void> {
    if (this.server?.connected) {
      this.server.disconnect();
    }
    this._cleanupConnection();
    this._notifyConnection(false);
  }

  /** Send a structured JSON message to the ESP32 */
  async send(message: OutboundMessage): Promise<void> {
    if (!this.rxCharacteristic) {
      throw new Error('Not connected to B.A.N. device.');
    }

    // Serialize to JSON string with terminator
    const json = JSON.stringify(message) + MSG_TERMINATOR;
    const encoded = new TextEncoder().encode(json);

    // Chunk the payload into BLE_CHUNK_SIZE packets
    for (let offset = 0; offset < encoded.length; offset += BLE_CHUNK_SIZE) {
      const chunk = encoded.slice(offset, offset + BLE_CHUNK_SIZE);
      await this.rxCharacteristic.writeValueWithoutResponse(chunk);
    }
  }

  /** Attempt to reconnect using the same device */
  async reconnect(): Promise<void> {
    if (!this.device) {
      throw new Error('No device to reconnect to. Please connect first.');
    }
    if (this.server?.connected) return;

    try {
      this.server = await this.device.gatt!.connect();
      const service = await this.server.getPrimaryService(BAN_SERVICE_UUID);

      this.txCharacteristic = await service.getCharacteristic(BAN_TX_UUID);
      await this.txCharacteristic.startNotifications();
      this.txCharacteristic.addEventListener(
        'characteristicvaluechanged',
        this._handleIncoming.bind(this)
      );

      this.rxCharacteristic = await service.getCharacteristic(BAN_RX_UUID);

      this._notifyConnection(true);
    } catch (err) {
      const msg = err instanceof Error ? err.message : String(err);
      this._notifyError(`Reconnect failed: ${msg}`);
      throw err;
    }
  }

  // ── Event subscriptions ─────────────────────────────────────

  onMessage(handler: MessageHandler): () => void {
    this.messageHandlers.push(handler);
    return () => {
      this.messageHandlers = this.messageHandlers.filter(h => h !== handler);
    };
  }

  onConnection(handler: ConnectionHandler): () => void {
    this.connectionHandlers.push(handler);
    return () => {
      this.connectionHandlers = this.connectionHandlers.filter(h => h !== handler);
    };
  }

  onError(handler: ErrorHandler): () => void {
    this.errorHandlers.push(handler);
    return () => {
      this.errorHandlers = this.errorHandlers.filter(h => h !== handler);
    };
  }

  // ── Private: incoming data handling ────────────────────────

  private _handleIncoming(event: Event): void {
    const target = event.target as BluetoothRemoteGATTCharacteristic;
    const value = target.value!;
    const chunk = new TextDecoder().decode(value);

    this.receiveBuffer += chunk;

    // Check for message terminator
    const terminatorIdx = this.receiveBuffer.indexOf(MSG_TERMINATOR);
    if (terminatorIdx >= 0) {
      const jsonStr = this.receiveBuffer.slice(0, terminatorIdx);
      this.receiveBuffer = this.receiveBuffer.slice(terminatorIdx + 1);

      try {
        const msg = JSON.parse(jsonStr) as InboundMessage;
        this.messageHandlers.forEach(h => h(msg));
      } catch {
        this._notifyError(`Invalid JSON from ESP32: ${jsonStr.slice(0, 100)}`);
      }
    }
  }

  private _handleDisconnect(): void {
    this._cleanupConnection();
    this._notifyConnection(false);
    this._notifyError('B.A.N. connection was lost.');
  }

  private _cleanupConnection(): void {
    this.txCharacteristic = null;
    this.rxCharacteristic = null;
    this.receiveBuffer = '';
  }

  private _notifyConnection(connected: boolean): void {
    this.connectionHandlers.forEach(h => h(connected));
  }

  private _notifyError(error: string): void {
    this.errorHandlers.forEach(h => h(error));
  }
}

// Export a singleton instance
export const bluetoothService = new BLEBluetoothService();
