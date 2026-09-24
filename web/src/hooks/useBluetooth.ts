// ============================================================
// B.A.N. — hooks/useBluetooth.ts
// React hook for managing BLE connection state
// ============================================================

import { useState, useEffect, useCallback, useRef } from 'react';
import { bluetoothService } from '../services/bluetooth';
import type { InboundMessage } from '../types/protocol';
import type { ConnectionState } from '../types/device';

export interface UseBluetoothReturn {
  connectionState: ConnectionState;
  isSupported: boolean;
  lastError: string | null;
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  reconnect: () => Promise<void>;
  send: (msg: object) => Promise<void>;
  onMessage: (handler: (msg: InboundMessage) => void) => () => void;
  clearError: () => void;
}

export function useBluetooth(): UseBluetoothReturn {
  const [connectionState, setConnectionState] = useState<ConnectionState>('disconnected');
  const [lastError, setLastError] = useState<string | null>(null);
  const isSupported = bluetoothService.isSupported();
  const messageHandlersRef = useRef<Array<(msg: InboundMessage) => void>>([]);

  useEffect(() => {
    // Subscribe to connection changes
    const unsubConn = bluetoothService.onConnection((connected) => {
      setConnectionState(connected ? 'connected' : 'disconnected');
    });

    // Subscribe to errors
    const unsubErr = bluetoothService.onError((error) => {
      setLastError(error);
      if (connectionState === 'connecting' || connectionState === 'reconnecting') {
        setConnectionState('disconnected');
      }
    });

    // Subscribe to messages — dispatch to all registered handlers
    const unsubMsg = bluetoothService.onMessage((msg) => {
      messageHandlersRef.current.forEach(h => h(msg));
    });

    return () => {
      unsubConn();
      unsubErr();
      unsubMsg();
    };
  }, [connectionState]);

  const connect = useCallback(async () => {
    setLastError(null);
    setConnectionState('connecting');
    try {
      await bluetoothService.connect();
      setConnectionState('connected');
    } catch {
      setConnectionState('disconnected');
    }
  }, []);

  const disconnect = useCallback(async () => {
    await bluetoothService.disconnect();
    setConnectionState('disconnected');
  }, []);

  const reconnect = useCallback(async () => {
    setLastError(null);
    setConnectionState('reconnecting');
    try {
      await bluetoothService.reconnect();
      setConnectionState('connected');
    } catch {
      setConnectionState('disconnected');
    }
  }, []);

  const send = useCallback(async (msg: object) => {
    await bluetoothService.send(msg as Parameters<typeof bluetoothService.send>[0]);
  }, []);

  const onMessage = useCallback((handler: (msg: InboundMessage) => void) => {
    messageHandlersRef.current.push(handler);
    return () => {
      messageHandlersRef.current = messageHandlersRef.current.filter(h => h !== handler);
    };
  }, []);

  const clearError = useCallback(() => setLastError(null), []);

  return {
    connectionState,
    isSupported,
    lastError,
    connect,
    disconnect,
    reconnect,
    send,
    onMessage,
    clearError,
  };
}
