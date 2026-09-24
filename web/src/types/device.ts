// ============================================================
// B.A.N. — types/device.ts
// Device state and status types
// ============================================================

export type CharacterState =
  | 'offline'
  | 'connecting'
  | 'idle'
  | 'listening'
  | 'thinking'
  | 'responding'
  | 'error';

export type CharacterMood =
  | 'idle'
  | 'wave'
  | 'talking'
  | 'happy'
  | 'wondering'
  | 'shock'
  | 'angry'
  | 'sad'
  | 'walking'
  | 'running';

export type ConnectionState =
  | 'disconnected'
  | 'connecting'
  | 'connected'
  | 'reconnecting';

export interface DeviceStatus {
  esp32: boolean;
  bluetooth: boolean;
  wifi: boolean;
  sdCard: boolean;
  aiBackend: boolean;
  memory: boolean;
  freeHeap?: number;
  uptime?: string;
  firmware?: string;
  deviceName?: string;
  sdTotal?: number;
  sdUsed?: number;
  memoryCount?: number;
}

export interface BanSettings {
  autoMemory: boolean;
  conversationLogging: boolean;
  aiBackend: string;
  aiEndpoint: string;
  debugMode: boolean;
  theme: 'dark' | 'light';
  characterSize: 'small' | 'medium' | 'large';
  animations: boolean;
  compactMode: boolean;
}
