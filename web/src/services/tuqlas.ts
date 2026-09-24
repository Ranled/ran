// ============================================================
// R.A.N. — tuqlas.ts
// Tuqlas AI Chatbot Bridge & Character Mood Alignment
// Synchronizes Tuqlas chatbot responses with R.A.N. character
// animations, moods, TTS voice, and speech bubbles.
// ============================================================

import type { CharacterMood } from '../types/device';

export interface TuqlasBridgeCallbacks {
  onBotMessage: (text: string, mood: CharacterMood) => void;
  onUserMessage?: (text: string) => void;
}

// Analyze sentiment and tone of incoming bot reply to align R.A.N.'s mood
export function detectMoodFromText(text: string): CharacterMood {
  const lower = text.toLowerCase();

  if (/\b(sorry|unfortunately|sadly|apologies|regret|unhappy|depressed|cry|tear|loss)\b/.test(lower)) {
    return 'sad';
  }
  if (/\b(error|bug|issue|fail|failed|broken|angry|grrr|glitch|bad)\b/.test(lower)) {
    return 'angry';
  }
  if (/\b(whoa|wow|shock|shocking|amazing|unbelievable|omg|alert|caution|surprise|⚡)\b/.test(lower)) {
    return 'shock';
  }
  if (/\b(hello|hey|hi|welcome|greetings|waves|morning|evening|👋)\b/.test(lower)) {
    return 'wave';
  }
  if (/\b(run|fast|speed|quick|hurry|sprint|dash|rush|boost|🏃)\b/.test(lower)) {
    return 'running';
  }
  if (/\b(walk|step|slow|careful|methodical|stroll|🚶)\b/.test(lower)) {
    return 'walking';
  }
  if (/\b(why|how|what if|wonder|curious|perhaps|maybe|consider|think|analyzing|investigating|🤔)\b/.test(lower)) {
    return 'wondering';
  }
  if (/\b(great|awesome|excellent|fantastic|haha|glad|congrats|congratulations|success|happy|love|nice|perfect|cool|win|😄|😊|✨|🎉)\b/.test(lower)) {
    return 'happy';
  }

  return 'talking';
}

class TuqlasService {
  private observer: MutationObserver | null = null;
  private lastProcessedText: string = '';
  private isProcessing = false;

  // Initialize observer on the Tuqlas widget DOM elements
  public initBridge(callbacks: TuqlasBridgeCallbacks) {
    if (typeof window === 'undefined') return;

    let attempts = 0;
    const findContainer = () => {
      const msgs = document.getElementById('tuqlas-msgs');
      if (msgs) {
        this.observeMessages(msgs, callbacks);
      } else if (attempts < 30) {
        attempts++;
        setTimeout(findContainer, 500);
      }
    };

    findContainer();
  }

  private observeMessages(msgsContainer: HTMLElement, callbacks: TuqlasBridgeCallbacks) {
    if (this.observer) {
      this.observer.disconnect();
    }

    let debounceTimer: any = null;

    this.observer = new MutationObserver(() => {
      clearTimeout(debounceTimer);
      // Debounce to allow streaming chunks to complete
      debounceTimer = setTimeout(() => {
        const botMessages = msgsContainer.querySelectorAll('.tql-bot');
        if (botMessages.length === 0) return;

        const latest = botMessages[botMessages.length - 1];
        const rawText = latest.textContent?.trim() || '';

        // Ignore loading dots "..."
        if (!rawText || rawText === '...' || rawText === this.lastProcessedText) {
          return;
        }

        // If the widget outputs the domain restriction notice, treat it as a warning
        if (rawText.includes("isn't available on this website")) {
          callbacks.onBotMessage(rawText, 'shock');
          this.lastProcessedText = rawText;
          return;
        }

        this.lastProcessedText = rawText;
        const mood = detectMoodFromText(rawText);
        callbacks.onBotMessage(rawText, mood);
      }, 400);
    });

    this.observer.observe(msgsContainer, {
      childList: true,
      subtree: true,
      characterData: true,
    });
  }

  // Programmatically forward a query into Tuqlas widget input if present
  public sendToTuqlasWidget(message: string): boolean {
    const input = document.getElementById('tuqlas-input') as HTMLInputElement | null;
    const form = document.getElementById('tuqlas-form') as HTMLFormElement | null;

    if (input && form) {
      input.value = message;
      form.dispatchEvent(new Event('submit', { bubbles: true, cancelable: true }));
      return true;
    }
    return false;
  }
}

export const tuqlasService = new TuqlasService();
