// ============================================================
// B.A.N. — types/protocol.ts
// BLE JSON protocol message types
// ============================================================

// Messages FROM Web → ESP32
export type OutboundMessageType =
  | 'chat'
  | 'command'
  | 'memory_save'
  | 'memory_delete'
  | 'memory_clear'
  | 'memory_search';

// Messages FROM ESP32 → Web
export type InboundMessageType =
  | 'response'
  | 'status'
  | 'memory_list'
  | 'memory_saved'
  | 'memory_deleted'
  | 'personality'
  | 'error'
  | 'ack';

// Outbound commands
export type BanCommand =
  | 'GET_STATUS'
  | 'GET_PERSONALITY'
  | 'GET_MEMORY'
  | 'RELOAD_PERSONALITY'
  | 'GET_CONVERSATION'
  | 'CLEAR_CONVERSATION';

// ── Outbound message shapes ───────────────────────────────────

export interface ChatOutbound {
  type: 'chat';
  message: string;
}

export interface CommandOutbound {
  type: 'command';
  command: BanCommand;
}

export interface MemorySaveOutbound {
  type: 'memory_save';
  category: string;
  key: string;
  value: string;
}

export interface MemoryDeleteOutbound {
  type: 'memory_delete';
  id: number;
}

export interface MemorySearchOutbound {
  type: 'memory_search';
  keyword: string;
}

export type OutboundMessage =
  | ChatOutbound
  | CommandOutbound
  | MemorySaveOutbound
  | MemoryDeleteOutbound
  | MemorySearchOutbound;

// ── Inbound message shapes ────────────────────────────────────

export interface ResponseInbound {
  type: 'response';
  message: string;
}

export interface StatusInbound {
  type: 'status';
  esp32: boolean;
  bluetooth: boolean;
  wifi: boolean;
  sd: boolean;
  ai: boolean;
  memory: boolean;
  freeHeap?: number;
  uptime?: string;
  firmware?: string;
  memoryCount?: number;
}

export interface MemoryListInbound {
  type: 'memory_list';
  memories: Array<{
    id: number;
    category: string;
    key: string;
    value: string;
    created: string;
  }>;
}

export interface MemorySavedInbound {
  type: 'memory_saved';
  id: number;
}

export interface MemoryDeletedInbound {
  type: 'memory_deleted';
  id: number;
}

export interface PersonalityInbound {
  type: 'personality';
  name: string;
  content: string;
}

export interface ErrorInbound {
  type: 'error';
  code: string;
  message: string;
}

export interface AckInbound {
  type: 'ack';
  command: string;
}

export type InboundMessage =
  | ResponseInbound
  | StatusInbound
  | MemoryListInbound
  | MemorySavedInbound
  | MemoryDeletedInbound
  | PersonalityInbound
  | ErrorInbound
  | AckInbound;
