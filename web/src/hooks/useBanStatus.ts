// ============================================================
// B.A.N. — hooks/useBanStatus.ts
// Device status management hook
// ============================================================

import { useState, useCallback } from 'react';
import type { DeviceStatus, CharacterState } from '../types/device';
import type { StatusInbound } from '../types/protocol';

const DEFAULT_STATUS: DeviceStatus = {
  esp32: false,
  bluetooth: false,
  wifi: false,
  sdCard: false,
  aiBackend: false,
  memory: false,
};

export interface UseBanStatusReturn {
  status: DeviceStatus;
  characterState: CharacterState;
  updateFromStatusMessage: (msg: StatusInbound) => void;
  setCharacterState: (state: CharacterState) => void;
  resetStatus: () => void;
}

export function useBanStatus(): UseBanStatusReturn {
  const [status, setStatus] = useState<DeviceStatus>(DEFAULT_STATUS);
  const [characterState, setCharacterState] = useState<CharacterState>('offline');

  const updateFromStatusMessage = useCallback((msg: StatusInbound) => {
    setStatus({
      esp32: msg.esp32,
      bluetooth: msg.bluetooth,
      wifi: msg.wifi,
      sdCard: msg.sd,
      aiBackend: msg.ai,
      memory: msg.memory,
      freeHeap: msg.freeHeap,
      uptime: msg.uptime,
      firmware: msg.firmware,
      memoryCount: msg.memoryCount,
    });
  }, []);

  const resetStatus = useCallback(() => {
    setStatus(DEFAULT_STATUS);
    setCharacterState('offline');
  }, []);

  return {
    status,
    characterState,
    updateFromStatusMessage,
    setCharacterState,
    resetStatus,
  };
}
