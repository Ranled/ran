// ============================================================
// B.A.N. — App.tsx
// Root application — wires all modules together:
//   Bluetooth ↔ State ↔ Components
// ============================================================

import { useState, useEffect, useRef, useCallback } from 'react';
import './index.css';

import { useBluetooth } from './hooks/useBluetooth';
import { useChat }      from './hooks/useChat';
import { useBanStatus } from './hooks/useBanStatus';
import { storage }      from './services/storage';
import { speechService } from './services/speech';
import { generateRanReply } from './services/ranAi';
import { tuqlasService } from './services/tuqlas';

import CharacterPanel from './components/CharacterPanel';
import ChatWindow     from './components/ChatWindow';
import MemoryPanel    from './components/MemoryPanel';
import SettingsPanel  from './components/SettingsPanel';

import type { InboundMessage } from './types/protocol';
import type { MemoryRecord, MemoryCategory } from './types/memory';
import type { BanSettings, CharacterMood } from './types/device';

const COMMAND_PREFIXES = ['/help','/status','/memory','/remember','/search','/forget','/personality','/reload','/device','/storage','/time'];

export default function App() {
  const bt     = useBluetooth();
  const chat   = useChat();
  const banSt  = useBanStatus();

  const [activeTab, setActiveTab] = useState<'chat' | 'memory' | 'settings'>('chat');
  const [isThinking, setIsThinking] = useState(false);
  const [currentMood, setCurrentMood] = useState<CharacterMood>('wave');
  const [bubbleText, setBubbleText] = useState<string | null>("Hello! I'm R.A.N. ✨");
  const [memories, setMemories] = useState<MemoryRecord[]>(() => storage.getCachedMemories());
  const [memLoading, setMemLoading] = useState(false);
  const [personality, setPersonality] = useState<{ name: string; content: string } | null>(() => storage.getCachedPersonality());
  const [settings, setSettings] = useState<BanSettings>(() => storage.loadSettings());
  const [lastRawPacket, setLastRawPacket] = useState<string>('');

  // Initial wave greeting timer with R.A.N. voice
  useEffect(() => {
    const greetingText = "Hello Master Raian! I am R.A.N., your personal AI assistant.";
    const t = setTimeout(() => {
      speechService.speak(
        greetingText,
        () => setCurrentMood('talking'),
        () => {
          setCurrentMood('wave');
          setTimeout(() => {
            setCurrentMood('idle');
            setBubbleText(null);
          }, 3000);
        }
      );
    }, 1200);
    return () => clearTimeout(t);
  }, []);

  // ── Tuqlas AI Chatbot Synchronizer ───────────────────────
  useEffect(() => {
    tuqlasService.initBridge({
      onBotMessage: (text: string, mood: CharacterMood) => {
        // When Tuqlas outputs a response, align R.A.N.'s character mood!
        setCurrentMood('talking');
        const shortBubble = text.length > 45 ? text.slice(0, 42) + '…' : text;
        setBubbleText(shortBubble);

        // Speak the Tuqlas answer out loud via Web Speech API!
        speechService.speak(
          text,
          () => setCurrentMood('talking'),
          () => {
            setCurrentMood(mood);
            setTimeout(() => {
              setCurrentMood('idle');
              setBubbleText(null);
            }, 3500);
          }
        );
      },
    });
  }, []);

  // ── Track pending chat message id for status updates ──────
  const pendingMsgId = useRef<string | null>(null);

  // ── Handle settings changes ───────────────────────────────
  const handleSettingsChange = useCallback((s: BanSettings) => {
    setSettings(s);
    storage.saveSettings(s);
  }, []);

  // ── Handle all inbound BLE messages ──────────────────────
  useEffect(() => {
    const unsub = bt.onMessage((msg: InboundMessage) => {
      setLastRawPacket(JSON.stringify(msg));

      switch (msg.type) {
        case 'response': {
          setIsThinking(false);
          chat.addBanMessage(msg.message);
          setCurrentMood('talking');
          setBubbleText(null);
          if (pendingMsgId.current) {
            chat.updateMessageStatus(pendingMsgId.current, 'sent');
            pendingMsgId.current = null;
          }
          // Speak R.A.N. response aloud!
          speechService.speak(
            msg.message,
            () => setCurrentMood('talking'),
            () => {
              banSt.setCharacterState('idle');
              setCurrentMood('idle');
            }
          );
          break;
        }

        case 'status': {
          banSt.updateFromStatusMessage(msg);
          if (banSt.characterState === 'connecting' || banSt.characterState === 'offline') {
            banSt.setCharacterState('idle');
            setCurrentMood('idle');
          }
          break;
        }

        case 'memory_list': {
          const records = msg.memories.map(m => ({
            id: m.id,
            category: m.category as MemoryCategory,
            key: m.key,
            value: m.value,
            created: m.created,
          }));
          setMemories(records);
          storage.cacheMemories(records);
          setMemLoading(false);
          break;
        }

        case 'memory_saved': {
          chat.addSystemMessage(`Memory #${msg.id} saved.`);
          requestMemory();
          break;
        }

        case 'memory_deleted': {
          chat.addSystemMessage(`Memory #${msg.id} deleted.`);
          setMemories(prev => prev.filter(m => m.id !== msg.id));
          break;
        }

        case 'personality': {
          setPersonality({ name: msg.name, content: msg.content });
          storage.cachePersonality(msg.name, msg.content);
          break;
        }

        case 'error': {
          setIsThinking(false);
          banSt.setCharacterState('error');
          setCurrentMood('shock');
          setBubbleText(`Error [${msg.code}]`);
          chat.addSystemMessage(`Error [${msg.code}]: ${msg.message}`);
          if (pendingMsgId.current) {
            chat.updateMessageStatus(pendingMsgId.current, 'error');
            pendingMsgId.current = null;
          }
          setTimeout(() => {
            banSt.setCharacterState('idle');
            setCurrentMood('idle');
            setBubbleText(null);
          }, 3000);
          break;
        }

        default:
          break;
      }
    });

    return unsub;
  }, [bt, chat, banSt]);

  // ── On connection established ─────────────────────────────
  useEffect(() => {
    if (bt.connectionState === 'connected') {
      banSt.setCharacterState('connecting');
      setCurrentMood('running');
      setBubbleText('Connecting BLE…');
      chat.addSystemMessage('Connected to B.A.N. Requesting status…');

      // Request status + personality + memory on connect
      setTimeout(async () => {
        try {
          await bt.send({ type: 'command', command: 'GET_STATUS' });
          await bt.send({ type: 'command', command: 'GET_PERSONALITY' });
          requestMemory();
          chat.addBanMessage("Hello, Master. I'm online and ready.");
          banSt.setCharacterState('idle');
          setCurrentMood('wave');
          setBubbleText("Connected! Hello Master Raian! ✨");
          setTimeout(() => {
            setCurrentMood('idle');
            setBubbleText(null);
          }, 3500);
        } catch { /* handled by error handler */ }
      }, 600);

    } else if (bt.connectionState === 'disconnected') {
      banSt.resetStatus();
      setIsThinking(false);
      chat.addSystemMessage('Disconnected from B.A.N.');
      setCurrentMood('idle');
      setBubbleText(null);
    }
  }, [bt.connectionState]);

  // ── Request memory from ESP32 ─────────────────────────────
  const requestMemory = useCallback(async () => {
    setMemLoading(true);
    try {
      await bt.send({ type: 'command', command: 'GET_MEMORY' });
    } catch {
      setMemLoading(false);
    }
  }, [bt]);

  // ── Handle sending a chat message ─────────────────────────
  const handleSend = useCallback(async (text: string) => {
    const userMsg = chat.addUserMessage(text);
    pendingMsgId.current = userMsg.id;
    setIsThinking(true);
    banSt.setCharacterState('thinking');
    setCurrentMood('wondering');
    setBubbleText('Thinking…');

    // 1. If connected to ESP32 hardware via Web Bluetooth, send over BLE
    if (bt.connectionState === 'connected') {
      try {
        await bt.send({ type: 'chat', message: text });
      } catch (err) {
        setIsThinking(false);
        banSt.setCharacterState('error');
        setCurrentMood('shock');
        setBubbleText('Send failed');
        chat.updateMessageStatus(userMsg.id, 'error');
        chat.addSystemMessage('Failed to send message to ESP32. Falling back to local companion mode.');
        setTimeout(() => {
          banSt.setCharacterState('idle');
          setCurrentMood('idle');
          setBubbleText(null);
        }, 3000);
      }
    } else {
      // 2. Web Companion Mode: R.A.N. replies with mood, animation, and voice!
      const reply = generateRanReply(text, memories, banSt.status, false);

      setTimeout(() => {
        setIsThinking(false);
        chat.addBanMessage(reply.text);
        chat.updateMessageStatus(userMsg.id, 'sent');
        pendingMsgId.current = null;
        setBubbleText(reply.bubble);

        // Speak response out loud using Web Speech API!
        speechService.speak(
          reply.text,
          () => {
            setCurrentMood('talking');
          },
          () => {
            setCurrentMood(reply.mood);
            setTimeout(() => {
              setCurrentMood('idle');
              setBubbleText(null);
            }, 3500);
          }
        );
      }, 500);
    }
  }, [bt, chat, banSt, memories]);

  const handleSpeak = useCallback((text: string) => {
    speechService.speak(
      text,
      () => setCurrentMood('talking'),
      () => setCurrentMood('idle')
    );
  }, []);

  // ── Memory actions ────────────────────────────────────────
  const handleMemorySave = useCallback(async (category: MemoryCategory, key: string, value: string) => {
    try {
      await bt.send({ type: 'memory_save', category, key, value });
    } catch {
      chat.addSystemMessage('Failed to save memory. Check connection.');
    }
  }, [bt, chat]);

  const handleMemoryDelete = useCallback(async (id: number) => {
    try {
      await bt.send({ type: 'memory_delete', id });
    } catch {
      chat.addSystemMessage('Failed to delete memory. Check connection.');
    }
  }, [bt, chat]);

  const handleMemorySearch = useCallback(async (keyword: string) => {
    try {
      await bt.send({ type: 'memory_search', keyword });
    } catch { /* ignore */ }
  }, [bt]);

  const handleReloadPersonality = useCallback(async () => {
    try {
      await bt.send({ type: 'command', command: 'GET_PERSONALITY' });
      chat.addSystemMessage('Reloading personality from ESP32…');
    } catch { /* ignore */ }
  }, [bt, chat]);

  // ── Render ────────────────────────────────────────────────
  return (
    <>
      <div className="app-bg" aria-hidden="true" />
      <div className="app-layout">

        {/* Left / Top: Animated Companion Stage */}
        <CharacterPanel
          characterState={banSt.characterState}
          connectionState={bt.connectionState}
          status={banSt.status}
          currentMood={currentMood}
          onMoodChange={setCurrentMood}
          onConnect={bt.connect}
          onDisconnect={bt.disconnect}
          isSupported={bt.isSupported}
          bubbleText={bubbleText}
          onQuickAction={handleSend}
        />

        {/* Right: Main content area */}
        {activeTab === 'chat' ? (
          <ChatWindow
            messages={chat.messages}
            isThinking={isThinking}
            connectionState={bt.connectionState}
            isSupported={bt.isSupported}
            lastError={bt.lastError}
            onSend={handleSend}
            onConnect={bt.connect}
            onClearError={bt.clearError}
            activeTab={activeTab}
            onTabChange={setActiveTab}
            onSpeak={handleSpeak}
          />
        ) : activeTab === 'memory' ? (
          <div className="chat-area">
            <div className="chat-header">
              <nav className="nav-tabs">
                {(['chat', 'memory', 'settings'] as const).map(tab => (
                  <button key={tab} className={`nav-tab${activeTab === tab ? ' active' : ''}`} onClick={() => setActiveTab(tab)} id={`nav-tab2-${tab}`}>
                    {tab === 'chat' && '💬 Chat'}
                    {tab === 'memory' && '🧠 Memory'}
                    {tab === 'settings' && '⚙️ Device'}
                  </button>
                ))}
              </nav>
            </div>
            <MemoryPanel
              memories={memories}
              onSave={handleMemorySave}
              onDelete={handleMemoryDelete}
              onSearch={handleMemorySearch}
              onRefresh={requestMemory}
              isLoading={memLoading}
            />
          </div>
        ) : (
          <div className="chat-area">
            <div className="chat-header">
              <nav className="nav-tabs">
                {(['chat', 'memory', 'settings'] as const).map(tab => (
                  <button key={tab} className={`nav-tab${activeTab === tab ? ' active' : ''}`} onClick={() => setActiveTab(tab)} id={`nav-tab3-${tab}`}>
                    {tab === 'chat' && '💬 Chat'}
                    {tab === 'memory' && '🧠 Memory'}
                    {tab === 'settings' && '⚙️ Device'}
                  </button>
                ))}
              </nav>
            </div>
            <SettingsPanel
              status={banSt.status}
              settings={settings}
              personality={personality}
              onReloadPersonality={handleReloadPersonality}
              onDisconnect={bt.disconnect}
              onSettingsChange={handleSettingsChange}
              lastRawPacket={lastRawPacket}
            />
          </div>
        )}
      </div>
    </>
  );
}
