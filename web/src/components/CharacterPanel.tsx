// ============================================================
// B.A.N. — CharacterPanel.tsx
// Animated companion stage matching the user's pod mockup:
// - Animated pixel art character on top
// - Dark capsule pedestal platform
// - Interactive mood selector & speech bubble
// - Device status & connection controls
// ============================================================

import { useState, useEffect } from 'react';
import './CharacterPanel.css';
import CharacterSprite, { MOOD_DEFINITIONS } from './CharacterSprite';
import { speechService } from '../services/speech';
import type { CharacterState, CharacterMood, DeviceStatus, ConnectionState } from '../types/device';

interface Props {
  characterState: CharacterState;
  connectionState: ConnectionState;
  status: DeviceStatus;
  currentMood: CharacterMood;
  onMoodChange: (mood: CharacterMood) => void;
  onConnect: () => void;
  onDisconnect: () => void;
  isSupported: boolean;
  bubbleText?: string | null;
  onQuickAction?: (actionText: string) => void;
}

const STATE_LABELS: Record<CharacterState, string> = {
  offline:    'Offline',
  connecting: 'Connecting...',
  idle:       'Online',
  listening:  'Listening',
  thinking:   'Thinking...',
  responding: 'Responding',
  error:      'Error',
};

const FUN_QUOTES = [
  "Hello, Master Raian! I'm R.A.N. ✨",
  "R.A.N. is standing by and ready to assist! 🚀",
  "ESP32 firmware standing by. ⚡",
  "Need me to remember something in SD card? 💾",
  "Say /help to see all R.A.N. commands!",
  "BLE link active and listening.",
];

export default function CharacterPanel({
  characterState,
  connectionState,
  status,
  currentMood,
  onMoodChange,
  onConnect,
  onDisconnect,
  isSupported,
  bubbleText,
  onQuickAction,
}: Props) {
  const isConnected = connectionState === 'connected';
  const isConnecting = connectionState === 'connecting' || connectionState === 'reconnecting';

  const [localBubble, setLocalBubble] = useState<string | null>(bubbleText || null);
  const [showMoodDrawer, setShowMoodDrawer] = useState(false);

  // Sync external bubble text
  useEffect(() => {
    if (bubbleText) {
      setLocalBubble(bubbleText);
    }
  }, [bubbleText]);

  // Handle clicking the character
  const handleCharacterClick = () => {
    // Pick a random greeting
    const randomQuote = FUN_QUOTES[Math.floor(Math.random() * FUN_QUOTES.length)];
    setLocalBubble(randomQuote);

    // Speak the quote aloud!
    speechService.speak(
      randomQuote,
      () => onMoodChange('talking'),
      () => {
        onMoodChange('wave');
        setTimeout(() => onMoodChange('idle'), 2500);
      }
    );

    // Clear bubble after 4.5 seconds
    setTimeout(() => {
      setLocalBubble(prev => (prev === randomQuote ? null : prev));
    }, 4500);
  };

  return (
    <aside className="companion-stage-container" aria-label="R.A.N. Companion">
      {/* ── Top Hero Stage: Character + Pedestal ── */}
      <div className="character-hero-stage">
        {/* Animated Character Sprite */}
        <div className="character-sprite-anchor">
          <CharacterSprite
            mood={currentMood}
            onClick={handleCharacterClick}
            bubbleText={localBubble}
            size="standard"
            interactive={true}
          />
        </div>

        {/* ── Dark Capsule Pedestal (from user's mockup) ── */}
        <div className="pedestal-pill" title="R.A.N. Stage Pedestal">
          <div className="pedestal-rim-glow" aria-hidden="true" />
          <div className="pedestal-surface">
            <span className="pedestal-dot" />
            <span className="pedestal-label">RAIAN AI NETWORK</span>
            <span className="pedestal-dot" />
          </div>
        </div>
      </div>

      {/* ── Stage Status & Quick Actions ── */}
      <div className="stage-controls-card">
        {/* Identity & Connection Header */}
        <div className="stage-header">
          <div className="stage-title-wrap">
            <div className="stage-name-row">
              <h2 className="stage-name">R.A.N.</h2>
              <span className={`status-pill ${characterState}`}>
                <span className={`status-indicator-dot ${characterState}`} />
                {STATE_LABELS[characterState]}
              </span>
            </div>
            <p className="stage-subtitle">Raian AI Network</p>
          </div>

          {/* Quick Connect / Disconnect button */}
          <div className="stage-connect-btn">
            {!isConnected && !isConnecting ? (
              <button
                className="btn btn-primary btn-sm btn-pulse"
                onClick={onConnect}
                disabled={!isSupported}
                id="btn-companion-connect"
              >
                ⬡ Connect
              </button>
            ) : isConnecting ? (
              <span className="badge badge-connecting">Connecting…</span>
            ) : (
              <button
                className="btn btn-ghost btn-sm"
                onClick={onDisconnect}
                id="btn-companion-disconnect"
              >
                Disconnect
              </button>
            )}
          </div>
        </div>

        {/* ── Quick Mood Selector ── */}
        <div className="mood-selector-bar">
          <div className="mood-bar-header">
            <span className="mood-bar-label">Character Mood & Animation</span>
            <button
              className="mood-toggle-link"
              onClick={() => setShowMoodDrawer(!showMoodDrawer)}
            >
              {showMoodDrawer ? 'Compact ▲' : 'All Moods ▼'}
            </button>
          </div>

          <div className={`mood-chips-scroll ${showMoodDrawer ? 'is-expanded' : ''}`}>
            {(Object.keys(MOOD_DEFINITIONS) as CharacterMood[]).map(mKey => {
              const item = MOOD_DEFINITIONS[mKey];
              const isSelected = currentMood === mKey;
              return (
                <button
                  key={mKey}
                  className={`mood-chip ${isSelected ? 'active' : ''}`}
                  onClick={() => {
                    onMoodChange(mKey);
                    setLocalBubble(`${item.icon} Mood: ${item.label}`);
                    setTimeout(() => setLocalBubble(null), 3000);
                  }}
                  title={`Switch to ${item.label} animation`}
                >
                  <span className="mood-chip-icon">{item.icon}</span>
                  <span className="mood-chip-name">{item.label}</span>
                </button>
              );
            })}
          </div>
        </div>

        {/* ── Quick Action Prompt Chips (Easy UX) ── */}
        {onQuickAction && (
          <div className="quick-actions-strip">
            <button
              className="quick-chip"
              onClick={() => {
                handleCharacterClick();
                onQuickAction('Hello R.A.N.!');
              }}
            >
              👋 Say Hello
            </button>
            <button
              className="quick-chip"
              onClick={() => onQuickAction('/status')}
            >
              ⚡ Status
            </button>
            <button
              className="quick-chip"
              onClick={() => onQuickAction('/help')}
            >
              ❓ Help
            </button>
            <button
              className="quick-chip"
              onClick={() => onQuickAction('/memory')}
            >
              🧠 Memory
            </button>
          </div>
        )}

        {/* ── Device Hardware Readout ── */}
        {isConnected && (
          <div className="companion-telemetry-grid">
            <div className="telemetry-pill">
              <span className="telemetry-key">ESP32</span>
              <span className={`telemetry-val ${status.esp32 ? 'ok' : 'fail'}`}>
                {status.esp32 ? 'Ready' : 'Offline'}
              </span>
            </div>
            <div className="telemetry-pill">
              <span className="telemetry-key">Bluetooth</span>
              <span className={`telemetry-val ${status.bluetooth ? 'ok' : 'fail'}`}>
                {status.bluetooth ? 'Connected' : 'Off'}
              </span>
            </div>
            <div className="telemetry-pill">
              <span className="telemetry-key">microSD</span>
              <span className={`telemetry-val ${status.sdCard ? 'ok' : 'fail'}`}>
                {status.sdCard ? 'Mounted' : 'Missing'}
              </span>
            </div>
            <div className="telemetry-pill">
              <span className="telemetry-key">AI</span>
              <span className={`telemetry-val ${status.aiBackend ? 'ok' : 'fail'}`}>
                {status.aiBackend ? 'Online' : 'Offline'}
              </span>
            </div>
            {status.freeHeap !== undefined && (
              <div className="telemetry-pill full-width">
                <span className="telemetry-key">Heap Memory</span>
                <span className="telemetry-val ok">
                  {Math.round(status.freeHeap / 1024)} KB Free
                </span>
              </div>
            )}
          </div>
        )}
      </div>
    </aside>
  );
}
