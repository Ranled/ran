// ============================================================
// B.A.N. — hooks/useChat.ts
// Chat message management
// ============================================================

import { useState, useCallback, useEffect } from 'react';
import type { ChatMessage } from '../types/chat';
import { storage } from '../services/storage';

let msgIdCounter = 0;
const generateId = () => `msg_${Date.now()}_${msgIdCounter++}`;

export interface UseChatReturn {
  messages: ChatMessage[];
  addUserMessage: (text: string) => ChatMessage;
  addBanMessage: (text: string) => ChatMessage;
  addSystemMessage: (text: string) => void;
  updateMessageStatus: (id: string, status: ChatMessage['status']) => void;
  clearMessages: () => void;
}

export function useChat(): UseChatReturn {
  const [messages, setMessages] = useState<ChatMessage[]>(() =>
    storage.loadConversation()
  );

  // Persist messages to localStorage whenever they change
  useEffect(() => {
    storage.saveConversation(messages);
  }, [messages]);

  const addUserMessage = useCallback((text: string): ChatMessage => {
    const msg: ChatMessage = {
      id: generateId(),
      sender: 'user',
      text,
      timestamp: Date.now(),
      status: 'sending',
    };
    setMessages(prev => [...prev, msg]);
    return msg;
  }, []);

  const addBanMessage = useCallback((text: string): ChatMessage => {
    const msg: ChatMessage = {
      id: generateId(),
      sender: 'ban',
      text,
      timestamp: Date.now(),
      status: 'sent',
    };
    setMessages(prev => [...prev, msg]);
    return msg;
  }, []);

  const addSystemMessage = useCallback((text: string): void => {
    const msg: ChatMessage = {
      id: generateId(),
      sender: 'system',
      text,
      timestamp: Date.now(),
      status: 'sent',
    };
    setMessages(prev => [...prev, msg]);
  }, []);

  const updateMessageStatus = useCallback(
    (id: string, status: ChatMessage['status']): void => {
      setMessages(prev =>
        prev.map(m => (m.id === id ? { ...m, status } : m))
      );
    },
    []
  );

  const clearMessages = useCallback((): void => {
    setMessages([]);
    storage.clearConversation();
  }, []);

  return {
    messages,
    addUserMessage,
    addBanMessage,
    addSystemMessage,
    updateMessageStatus,
    clearMessages,
  };
}
