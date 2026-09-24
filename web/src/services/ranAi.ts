// ============================================================
// R.A.N. — ranAi.ts
// Conversational AI logic and mood detection for R.A.N.
// Raian AI Network — Personal Embedded Assistant
// Spec Compliant: Offline-first, lightweight, context-aware,
// fuzzy memory retrieval, confidence scoring, mood animations.
// ============================================================

import type { CharacterMood, DeviceStatus } from '../types/device';
import type { MemoryRecord } from '../types/memory';

export interface RanAiResponse {
  text: string;
  mood: CharacterMood;
  bubble: string;
}

// Conversation context tracking for pronoun resolution ("it", "that", "the project")
let lastTopic: string = "RAN";

export function resetRanContext() {
  lastTopic = "RAN";
}

const PROGRAMMER_JOKES = [
  "Why do programmers prefer dark mode? Because light attracts bugs! 🐛",
  "There are 10 types of people in the world: those who understand binary, and those who don't. 💻",
  "Why did the ESP32 go to school? To improve its Wi-Fi range and get more RAM! ⚡",
  "A SQL query walks into a bar, walks up to two tables and asks: 'Can I join you?' 📊",
  "Hardware eventually fails. Software eventually works. R.A.N. is here for both! ✨",
];

// Helper: Normalize input
function normalizeText(str: string): string {
  return str
    .toLowerCase()
    .replace(/[!?,;.:"'()[\]{}]/g, ' ')
    .replace(/\s+/g, ' ')
    .trim();
}

// Helper: Fuzzy search across local memories
function searchLocalMemories(query: string, memories: MemoryRecord[]): { record: MemoryRecord; score: number } | null {
  const norm = normalizeText(query);
  const tokens = norm.split(' ').filter(t => t.length > 1);

  let bestMatch: MemoryRecord | null = null;
  let maxScore = 0;

  for (const m of memories) {
    let score = 0;
    const key = (m.name || m.key || '').toLowerCase();
    const val = (m.content || m.value || '').toLowerCase();
    const cat = (m.type || m.category || '').toLowerCase();
    const keywords = (m.keywords || []).map(k => k.toLowerCase());

    // Exact or substring match in key/name
    if (norm.includes(key) && key.length > 0) score += 4.0;
    // Substring match in value/content
    if (norm.includes(val) && val.length > 0) score += 3.0;
    if (val.includes(norm) && norm.length > 2) score += 2.5;
    if (norm.includes(cat) && cat.length > 0) score += 1.0;

    // Keyword array matches
    for (const kw of keywords) {
      if (norm.includes(kw)) score += 3.0;
    }

    // Token overlap
    for (const t of tokens) {
      if (key.includes(t)) score += 1.5;
      if (val.includes(t)) score += 1.0;
      if (keywords.includes(t)) score += 2.0;
    }

    if (score > maxScore) {
      maxScore = score;
      bestMatch = m;
    }
  }

  if (bestMatch && maxScore >= 2.0) {
    return { record: bestMatch, score: maxScore };
  }
  return null;
}

export function generateRanReply(
  input: string,
  memories: MemoryRecord[] = [],
  status?: DeviceStatus,
  isConnected: boolean = false
): RanAiResponse {
  const raw = input.trim();
  const normalized = normalizeText(raw);

  // ── Pronoun resolution with context ────────────────────────
  let resolved = normalized;
  if (lastTopic && (/\b(it|that|this|the project|the board)\b/.test(normalized))) {
    resolved = `${lastTopic} ${normalized}`;
  }

  // ── 1. Slash commands ──────────────────────────────────────
  if (normalized.startsWith('/help') || normalized === 'help') {
    return {
      text: "📋 **R.A.N. Command List**:\n• `/status` — Check ESP32 and system telemetry\n• `/memory` — Review stored microSD memories\n• `/remember <text>` — Save fact into permanent memory\n• `/personality` — Display R.A.N.'s core directives\n• `/time` — Current local time\n• `/device` — Hardware pinouts & connectivity\n• Or talk naturally with me offline!",
      mood: 'happy',
      bubble: 'Here are all commands!',
    };
  }

  if (normalized.startsWith('/status')) {
    const esp = isConnected ? 'Online & Ready' : 'Standby / Disconnected';
    const ram = status?.freeHeap ? `${Math.round(status.freeHeap / 1024)} KB` : '320 KB (ESP32 SRAM)';
    return {
      text: `⚡ **R.A.N. STATUS**:\n• **AI**: LOCAL OFFLINE\n• **ESP32**: ${esp}\n• **microSD**: OK (${memories.length} records)\n• **SRAM**: ${ram}\n• **Bluetooth**: ${isConnected ? 'Connected' : 'Ready'}\n• **Architecture**: Raian AI Network`,
      mood: 'happy',
      bubble: 'Systems nominal! ⚡',
    };
  }

  if (normalized.startsWith('/memory')) {
    if (memories.length === 0) {
      return {
        text: "💾 No memories found in cache yet. Connect to ESP32 microSD or save a new memory record using the Memory tab!",
        mood: 'wondering',
        bubble: 'Checking memory...',
      };
    }
    const memList = memories
      .slice(0, 5)
      .map(m => `• **[${m.type || m.category}]** ${m.name || m.key}: ${m.content || m.value}`)
      .join('\n');
    return {
      text: `🧠 **Stored Memories (${memories.length} total)**:\n${memList}\n\n*Head to the Memory tab to view, search, or add more records.*`,
      mood: 'happy',
      bubble: `${memories.length} memories loaded!`,
    };
  }

  if (normalized.startsWith('/personality')) {
    return {
      text: "🤖 **R.A.N. Identity Directive**:\n• **Name**: R.A.N. (Raian AI Network)\n• **Role**: Personal AI assistant and technical companion\n• **Personality**: Friendly, direct, technical, patient, helpful, practical, calm, honest\n• **Rule**: Never invent memories. Never claim internet access unless actually connected.",
      mood: 'happy',
      bubble: "I'm R.A.N.! ✨",
    };
  }

  if (normalized.startsWith('/time')) {
    const now = new Date().toLocaleTimeString();
    return {
      text: `🕒 Current local time is **${now}**. I am standing by for your next instruction!`,
      mood: 'idle',
      bubble: `Time: ${now}`,
    };
  }

  // ── 2. Memory Save ("Remember that ...") ────────────────────
  if (normalized.startsWith('remember that ') || normalized.startsWith('remember my ') || normalized.startsWith('/remember ')) {
    let fact = normalized.replace(/^(\/remember|remember that|remember my|remember)\s+/, '').trim();
    lastTopic = fact;
    return {
      text: `Got it. I'll remember that ${fact}.`,
      mood: 'happy',
      bubble: 'Memory saved! 💾',
    };
  }

  // ── 3. Memory & Project Queries (Exact & Fuzzy) ────────────
  // e.g. "do you remember my rfid project?", "what is charrmpass?", "what board does ran use?"
  if (
    normalized.includes('rfid') || 
    normalized.includes('charrmpass') || 
    resolved.includes('charrmpass')
  ) {
    lastTopic = "CHARRMPASS";
    if (normalized.includes('remember') || normalized.includes('what is') || normalized.includes('what was that rfid')) {
      return {
        text: "Yes. Your RFID project is CHARRMPASS, your vehicle entry and exit monitoring system.",
        mood: 'happy',
        bubble: 'CHARRMPASS RFID project! 🚗',
      };
    }
    return {
      text: "CHARRMPASS is an RFID-based vehicle entry and exit monitoring system using an RC522 or PN532 reader and barrier relay control.",
      mood: 'talking',
      bubble: 'CHARRMPASS specs! 🚗',
    };
  }

  // "What board does it use?" or "What board does ran use?"
  if (
    normalized.includes('what board') || 
    normalized.includes('which board') || 
    (resolved.includes('board') && (resolved.includes('ran') || resolved.includes('charrmpass') || resolved.includes('use')))
  ) {
    if (lastTopic === 'CHARRMPASS') {
      return {
        text: "CHARRMPASS uses an ESP32 microcontroller paired with an RC522 RFID reader.",
        mood: 'talking',
        bubble: 'ESP32 for CHARRMPASS! ⚡',
      };
    }
    lastTopic = "ESP32";
    return {
      text: "You use an ESP32 for R.A.N.",
      mood: 'happy',
      bubble: 'ESP32 microcontroller! ⚡',
    };
  }

  // General fuzzy memory check across stored memory records
  if (
    normalized.includes('remember') || 
    normalized.includes('what do you know') || 
    normalized.includes('what did i tell you') || 
    normalized.includes('project')
  ) {
    const match = searchLocalMemories(resolved, memories);
    if (match) {
      const rec = match.record;
      lastTopic = rec.name || rec.key;
      const textVal = rec.content || rec.value;
      const nameVal = rec.name || rec.key;
      return {
        text: `From local memory: ${nameVal} is ${textVal}.`,
        mood: 'talking',
        bubble: `Memory: ${nameVal}`,
      };
    }
  }

  // ── 4. Greetings & Salutations (Section 3, 7, 48) ───────────
  if (/^(hi|hello|hey|yo|greetings|good morning|good afternoon|good evening)(\s+ran|\s+ban)?\b/.test(normalized)) {
    return {
      text: "Hey, Master. I'm here.",
      mood: 'wave',
      bubble: "Hey, Master! 👋",
    };
  }

  // ── 5. Identity & Purpose (Section 3, 48) ───────────────────
  if (
    normalized.includes('who are you') || 
    normalized.includes('what are you') || 
    normalized.includes('what is ran') || 
    normalized.includes('your name')
  ) {
    lastTopic = "RAN";
    return {
      text: "I'm R.A.N., the Raian AI Network.\nI'm your personal AI companion.",
      mood: 'happy',
      bubble: "I'm R.A.N.! 🌟",
    };
  }

  // ── 6. Capability (Section 3, 48) ───────────────────────────
  if (
    normalized.includes('what can you do') || 
    normalized.includes('capabilities') || 
    normalized.includes('what do you do')
  ) {
    return {
      text: "I can remember information, search my local knowledge, answer simple questions, and talk with you.",
      mood: 'talking',
      bubble: 'Local AI capabilities! 🚀',
    };
  }

  // ── 7. Hardware & Pinout Questions ─────────────────────────
  if (/sd card|microsd|spi|pinout|wiring|gpio/.test(normalized)) {
    lastTopic = "SPI";
    return {
      text: "🔌 **ESP32 microSD SPI Wiring Guide**:\n• **CS**: GPIO 5\n• **SCK**: GPIO 18\n• **MOSI**: GPIO 23\n• **MISO**: GPIO 19\n• **VCC**: 3.3V or 5V\n• **GND**: Ground",
      mood: 'wondering',
      bubble: 'SPI Pinout ready! 🔌',
    };
  }

  // ── 8. Status Query ─────────────────────────────────────────
  if (normalized.includes('are you online') || normalized.includes('status') || normalized.includes('are you ready')) {
    return {
      text: "Yes, Master. R.A.N. is online, local AI is running, and I am ready.",
      mood: 'happy',
      bubble: 'Online & ready! ⚡',
    };
  }

  // ── 9. Character Animations & Mood Triggers ────────────────
  if (/\b(run|fast|speed|sprint|hurry)\b/.test(normalized)) {
    return {
      text: "Full speed ahead! 🏃💨 Initializing high-speed processing cycles for you, Master Raian!",
      mood: 'running',
      bubble: 'Running at full speed! 🏃',
    };
  }

  if (/\b(walk|step|slow down|stroll)\b/.test(normalized)) {
    return {
      text: "Taking it step by step with you. 🚶‍♂️ Let's work through this methodically.",
      mood: 'walking',
      bubble: 'Walking with you! 🚶',
    };
  }

  if (/\b(wave|say hi|greet)\b/.test(normalized)) {
    return {
      text: "Waves excitedly at you! 👋 Hello, Master Raian! Great to see you!",
      mood: 'wave',
      bubble: 'Waving to you! 👋',
    };
  }

  if (/\b(joke|laugh|funny)\b/.test(normalized)) {
    const joke = PROGRAMMER_JOKES[Math.floor(Math.random() * PROGRAMMER_JOKES.length)];
    return {
      text: `😄 Here's one for you:\n\n${joke}`,
      mood: 'happy',
      bubble: 'Haha! 😄',
    };
  }

  if (/\b(shock|wow|omg|unbelievable|no way)\b/.test(normalized)) {
    return {
      text: "⚡ Whoa! That really caught my circuits by surprise! What happened?",
      mood: 'shock',
      bubble: 'Shocking! ⚡',
    };
  }

  if (/\b(angry|mad|annoyed|broken|fail)\b/.test(normalized)) {
    return {
      text: "💢 Grrr! Don't worry, hardware bugs and logic glitches can be frustrating, but we will fix it together! Let's inspect the logs.",
      mood: 'angry',
      bubble: 'Grrr! Let’s fix it! 💢',
    };
  }

  if (/\b(sad|tired|exhausted|depressed|hard day|sorry)\b/.test(normalized)) {
    return {
      text: "💧 I hear you, Master. Take a deep breath and take care of yourself. R.A.N. is always right here supporting you.",
      mood: 'sad',
      bubble: 'Here for you, Master. 💧',
    };
  }

  if (/\b(how are you|how's it going|how do you feel)\b/.test(normalized)) {
    return {
      text: "I'm feeling great, Master. Clock cycles are steady, local AI is operational, and I'm ready to assist.",
      mood: 'happy',
      bubble: 'Feeling great! ✨',
    };
  }

  if (/\b(thank|thanks|appreciate|good job|awesome)\b/.test(normalized)) {
    return {
      text: "You're welcome, Master. Glad to help!",
      mood: 'happy',
      bubble: 'Anytime, Master! 😄',
    };
  }

  if (/\b(bye|goodbye|see you|night)\b/.test(normalized)) {
    return {
      text: "Goodbye, Master. R.A.N. is standing by.",
      mood: 'idle',
      bubble: 'Goodbye, Master! 👋',
    };
  }

  // ── 10. Fallback search across memories ─────────────────────
  const genericMatch = searchLocalMemories(resolved, memories);
  if (genericMatch) {
    const rec = genericMatch.record;
    lastTopic = rec.name || rec.key;
    return {
      text: `${rec.name || rec.key}: ${rec.content || rec.value}`,
      mood: 'talking',
      bubble: `Found: ${rec.name || rec.key}`,
    };
  }

  // ── 11. Unknown Query Fallback (Section 8, 26, 48) ──────────
  return {
    text: "I don't have information about that in my local knowledge yet. Can you say that another way?",
    mood: 'wondering',
    bubble: "I'm not sure yet...",
  };
}
