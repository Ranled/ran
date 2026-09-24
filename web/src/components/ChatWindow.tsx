// ============================================================
// R.A.N. — ChatWindow.tsx
// Main chat interface: messages + light pill input + voice controls
// Designed to match the dark pod + light capsule companion aesthetics
// ============================================================

import { useRef, useEffect, useState, KeyboardEvent } from 'react';
import './ChatWindow.css';
import { speechService } from '../services/speech';
import type { ChatMessage } from '../types/chat';
import type { ConnectionState } from '../types/device';

interface Props {
  messages: ChatMessage[];
  isThinking: boolean;
  connectionState: ConnectionState;
  isSupported: boolean;
  lastError: string | null;
  onSend: (text: string) => void;
  onConnect: () => void;
  onClearError: () => void;
  activeTab: 'chat' | 'memory' | 'settings';
  onTabChange: (tab: 'chat' | 'memory' | 'settings') => void;
  onSpeak?: (text: string) => void;
}

const QUICK_PROMPTS = [
  { label: '👋 Say Hi to R.A.N.', text: 'Hello R.A.N.!' },
  { label: '⚡ Device Status', text: '/status' },
  { label: '🧠 Stored Memories', text: '/memory' },
  { label: '❓ Help Commands', text: '/help' },
  { label: '😄 Tell a Joke', text: 'Tell me a joke!' },
  { label: '🏃 Run Fast', text: 'Run as fast as you can!' },
];

export default function ChatWindow({
  messages,
  isThinking,
  connectionState,
  isSupported,
  lastError,
  onSend,
  onConnect,
  onClearError,
  activeTab,
  onTabChange,
  onSpeak,
}: Props) {
  const [inputText, setInputText] = useState('');
  const [voiceEnabled, setVoiceEnabled] = useState(speechService.getEnabled());
  const messagesEndRef = useRef<HTMLDivElement>(null);
  const textareaRef = useRef<HTMLTextAreaElement>(null);
  const isConnected = connectionState === 'connected';
  const isConnecting = connectionState === 'connecting' || connectionState === 'reconnecting';

  // Auto-scroll to latest message
  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages, isThinking]);

  // Auto-resize textarea
  const handleInput = () => {
    const ta = textareaRef.current;
    if (!ta) return;
    ta.style.height = 'auto';
    ta.style.height = Math.min(ta.scrollHeight, 100) + 'px';
  };

  const handleSend = () => {
    const text = inputText.trim();
    if (!text || isThinking) return;

    onSend(text);
    setInputText('');
    if (textareaRef.current) {
      textareaRef.current.style.height = 'auto';
    }
  };

  const handleKeyDown = (e: KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  const toggleVoice = () => {
    const next = !voiceEnabled;
    speechService.setEnabled(next);
    setVoiceEnabled(next);
    if (!next) {
      speechService.stop();
    }
  };

  return (
    <main className="chat-area" aria-label="R.A.N. Main Console">
      {/* ── Top Header Navigation ── */}
      <ChatHeader
        connectionState={connectionState}
        activeTab={activeTab}
        onTabChange={onTabChange}
        isConnected={isConnected}
      />

      {/* ── Connection Banner if offline ── */}
      {!isConnected && (
        <div className="connection-banner-strip">
          <div className="banner-left">
            <span className="banner-pulse-dot" />
            <div className="banner-text">
              <strong>ESP32 Hardware Offline</strong>
              <span className="banner-sub">R.A.N. is chatting in Web Companion mode. Click to link ESP32 Bluetooth.</span>
            </div>
          </div>
          <div className="banner-right">
            {lastError && (
              <button
                className="banner-err-dismiss"
                onClick={onClearError}
                title={lastError}
              >
                Clear Error
              </button>
            )}
            <button
              className="btn btn-primary btn-sm"
              onClick={onConnect}
              disabled={isConnecting || !isSupported}
              id="btn-banner-connect"
            >
              {isConnecting ? 'Pairing…' : '⬡ Connect ESP32'}
            </button>
          </div>
        </div>
      )}

      {/* ── Chat Messages Stream ── */}
      <section
        className="chat-messages"
        aria-label="Conversation"
        aria-live="polite"
        aria-atomic={false}
      >
        {messages.length === 0 && (
          <div className="empty-state">
            <div className="empty-state-badge">R.A.N. READY</div>
            <p className="empty-state-title">Raian AI Network Companion</p>
            <p className="text-sm text-muted">
              R.A.N. talks and replies with live mood animations! Click the sprite or chat below.
            </p>
          </div>
        )}

        {messages.map(msg => (
          <MessageRow
            key={msg.id}
            message={msg}
            onSpeak={onSpeak || ((txt) => speechService.speak(txt))}
          />
        ))}

        {/* Thinking animation */}
        {isThinking && (
          <div className="message-row ban" aria-label="R.A.N. is thinking">
            <span className="message-sender-label">R.A.N.</span>
            <div className="thinking-indicator">
              <div className="thinking-dots" aria-hidden="true">
                <span className="thinking-dot" />
                <span className="thinking-dot" />
                <span className="thinking-dot" />
              </div>
              <span className="thinking-label">R.A.N. is thinking…</span>
            </div>
          </div>
        )}

        <div ref={messagesEndRef} />
      </section>

      {/* ── Bottom Input Section (Matching the light pill in user's mockup) ── */}
      <div className="chat-bottom-dock">
        {/* Quick prompt suggestions */}
        <div className="quick-suggestions-bar">
          {QUICK_PROMPTS.map(p => (
            <button
              key={p.text}
              className="quick-prompt-pill"
              onClick={() => onSend(p.text)}
            >
              {p.label}
            </button>
          ))}
        </div>

        {/* Light pill input bar (matches the bottom light capsule in user's image) */}
        <div className="pod-light-pill-wrapper">
          <div className={`pod-light-pill-input ${isThinking ? 'disabled' : ''}`}>
            {/* Voice Mute / Unmute Toggle Button */}
            <button
              className={`pill-voice-btn ${voiceEnabled ? 'voice-on' : 'voice-off'}`}
              onClick={toggleVoice}
              title={voiceEnabled ? 'Voice output: ON (Click to mute)' : 'Voice output: MUTED (Click to speak)'}
              type="button"
            >
              {voiceEnabled ? '🔊' : '🔇'}
            </button>

            <span className="pill-terminal-symbol">❯</span>
            <textarea
              ref={textareaRef}
              className="pill-textarea"
              placeholder={
                isThinking
                  ? 'R.A.N. is thinking…'
                  : 'Message R.A.N. — or type /help'
              }
              value={inputText}
              onChange={e => setInputText(e.target.value)}
              onInput={handleInput}
              onKeyDown={handleKeyDown}
              disabled={isThinking}
              aria-label="Message input"
              rows={1}
            />

            <button
              className="pill-send-btn"
              onClick={handleSend}
              disabled={isThinking || !inputText.trim()}
              aria-label="Send message"
              id="btn-send"
              title="Send to R.A.N."
            >
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                <line x1="22" y1="2" x2="11" y2="13"/>
                <polygon points="22 2 15 22 11 13 2 9 22 2"/>
              </svg>
            </button>
          </div>
        </div>

        <p className="chat-input-hint">
          Enter to send · Shift+Enter for newline · Click R.A.N. to interact & hear him speak
        </p>
      </div>
    </main>
  );
}

// ── Sub-components ────────────────────────────────────────────

function ChatHeader({
  connectionState,
  activeTab,
  onTabChange,
  isConnected,
}: {
  connectionState: ConnectionState;
  activeTab: string;
  onTabChange: (tab: 'chat' | 'memory' | 'settings') => void;
  isConnected: boolean;
}) {
  const badgeClass =
    connectionState === 'connected' ? 'connected' :
    connectionState === 'connecting' || connectionState === 'reconnecting' ? 'connecting' :
    'disconnected';

  const badgeLabel =
    connectionState === 'connected' ? 'ESP32 Connected' :
    connectionState === 'connecting' ? 'Connecting…' :
    connectionState === 'reconnecting' ? 'Reconnecting…' :
    'Companion Mode';

  return (
    <header className="chat-header">
      <div className="chat-header-left">
        <nav className="nav-tabs" aria-label="Navigation">
          {(['chat', 'memory', 'settings'] as const).map(tab => (
            <button
              key={tab}
              className={`nav-tab${activeTab === tab ? ' active' : ''}`}
              onClick={() => onTabChange(tab)}
              aria-current={activeTab === tab ? 'page' : undefined}
              id={`nav-tab-${tab}`}
            >
              {tab === 'chat' && '💬 Chat'}
              {tab === 'memory' && '🧠 Memory'}
              {tab === 'settings' && '⚙️ Device'}
            </button>
          ))}
        </nav>
      </div>

      <div className="chat-header-right">
        <div className={`connection-badge ${badgeClass}`} role="status" aria-label={`Connection: ${badgeLabel}`}>
          <span className={`status-dot ${badgeClass === 'connected' ? 'online' : badgeClass === 'connecting' ? 'warning' : 'offline'}`} />
          {badgeLabel}
        </div>
      </div>
    </header>
  );
}

function MessageRow({
  message,
  onSpeak,
}: {
  message: ChatMessage;
  onSpeak: (text: string) => void;
}) {
  const [copied, setCopied] = useState(false);
  const time = new Date(message.timestamp).toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
  });

  const senderLabel =
    message.sender === 'user' ? 'You' :
    message.sender === 'ban'  ? 'R.A.N.' :
    'System';

  const handleCopy = () => {
    navigator.clipboard.writeText(message.text);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <article className={`message-row ${message.sender}`}>
      {message.sender !== 'system' && (
        <span className="message-sender-label">{senderLabel}</span>
      )}
      <div className="message-bubble-wrapper">
        <div className="message-bubble" role="text">
          {message.text}
        </div>
        {message.sender === 'ban' && (
          <div className="message-actions-row">
            <button
              className="message-action-btn"
              onClick={() => onSpeak(message.text)}
              title="Read aloud"
            >
              🔊
            </button>
            <button
              className="message-action-btn"
              onClick={handleCopy}
              title={copied ? 'Copied!' : 'Copy response'}
            >
              {copied ? '✓' : '⧉'}
            </button>
          </div>
        )}
      </div>
      <time className="message-timestamp" dateTime={new Date(message.timestamp).toISOString()}>
        {time}
      </time>
    </article>
  );
}
