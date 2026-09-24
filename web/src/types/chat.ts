// ============================================================
// B.A.N. — types/chat.ts
// Chat message types
// ============================================================

export type MessageSender = 'user' | 'ban' | 'system';
export type MessageStatus = 'sending' | 'sent' | 'error';

export interface ChatMessage {
  id: string;
  sender: MessageSender;
  text: string;
  timestamp: number;
  status?: MessageStatus;
}

export interface ConversationSession {
  id: string;
  startedAt: number;
  messages: ChatMessage[];
}
