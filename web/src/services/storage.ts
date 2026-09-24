// ============================================================
// B.A.N. — services/storage.ts
// Browser-side local storage for UI state caching.
// NOTE: ESP32 microSD is the authoritative persistent store.
//       This is only for UI caching (conversation history, etc.)
// ============================================================

import type { ChatMessage } from '../types/chat';
import type { MemoryRecord } from '../types/memory';
import type { BanSettings } from '../types/device';

const STORAGE_KEYS = {
  CONVERSATION: 'ban_conversation',
  MEMORIES: 'ban_memories_cache',
  SETTINGS: 'ban_settings',
  PERSONALITY: 'ban_personality',
};

const DEFAULT_SETTINGS: BanSettings = {
  autoMemory: false,
  conversationLogging: true,
  aiBackend: 'none',
  aiEndpoint: '',
  debugMode: false,
  theme: 'dark',
  characterSize: 'medium',
  animations: true,
  compactMode: false,
};

export const storage = {
  // ── Conversation ─────────────────────────────────────────────

  saveConversation(messages: ChatMessage[]): void {
    try {
      localStorage.setItem(STORAGE_KEYS.CONVERSATION, JSON.stringify(messages));
    } catch { /* Ignore storage quota errors */ }
  },

  loadConversation(): ChatMessage[] {
    try {
      const raw = localStorage.getItem(STORAGE_KEYS.CONVERSATION);
      return raw ? JSON.parse(raw) : [];
    } catch { return []; }
  },

  clearConversation(): void {
    localStorage.removeItem(STORAGE_KEYS.CONVERSATION);
  },

  // ── Memory cache ─────────────────────────────────────────────

  cacheMemories(memories: MemoryRecord[]): void {
    try {
      localStorage.setItem(STORAGE_KEYS.MEMORIES, JSON.stringify(memories));
    } catch { /* Ignore */ }
  },

  getCachedMemories(): MemoryRecord[] {
    try {
      const raw = localStorage.getItem(STORAGE_KEYS.MEMORIES);
      return raw ? JSON.parse(raw) : [];
    } catch { return []; }
  },

  clearMemoryCache(): void {
    localStorage.removeItem(STORAGE_KEYS.MEMORIES);
  },

  // ── Settings ─────────────────────────────────────────────────

  saveSettings(settings: BanSettings): void {
    try {
      localStorage.setItem(STORAGE_KEYS.SETTINGS, JSON.stringify(settings));
    } catch { /* Ignore */ }
  },

  loadSettings(): BanSettings {
    try {
      const raw = localStorage.getItem(STORAGE_KEYS.SETTINGS);
      return raw ? { ...DEFAULT_SETTINGS, ...JSON.parse(raw) } : DEFAULT_SETTINGS;
    } catch { return DEFAULT_SETTINGS; }
  },

  // ── Personality cache ────────────────────────────────────────

  cachePersonality(name: string, content: string): void {
    try {
      localStorage.setItem(STORAGE_KEYS.PERSONALITY, JSON.stringify({ name, content }));
    } catch { /* Ignore */ }
  },

  getCachedPersonality(): { name: string; content: string } | null {
    try {
      const raw = localStorage.getItem(STORAGE_KEYS.PERSONALITY);
      return raw ? JSON.parse(raw) : null;
    } catch { return null; }
  },
};
