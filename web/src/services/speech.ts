// ============================================================
// R.A.N. — speech.ts
// Voice synthesis for R.A.N. using Web Speech API
// ============================================================

class SpeechService {
  private isEnabled: boolean = true;
  private currentUtterance: SpeechSynthesisUtterance | null = null;
  private voice: SpeechSynthesisVoice | null = null;

  constructor() {
    if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
      this.initVoice();
      if (window.speechSynthesis.onvoiceschanged !== undefined) {
        window.speechSynthesis.onvoiceschanged = () => this.initVoice();
      }
    }
  }

  private initVoice() {
    if (typeof window === 'undefined' || !('speechSynthesis' in window)) return;
    const voices = window.speechSynthesis.getVoices();
    // Prefer friendly English voices
    const preferred = voices.find(v => 
      v.lang.startsWith('en') && 
      (v.name.includes('Natural') || v.name.includes('Google') || v.name.includes('Samantha') || v.name.includes('Daniel') || v.name.includes('Alex'))
    ) || voices.find(v => v.lang.startsWith('en')) || voices[0];

    if (preferred) {
      this.voice = preferred;
    }
  }

  public setEnabled(enabled: boolean) {
    this.isEnabled = enabled;
    if (!enabled) {
      this.stop();
    }
  }

  public getEnabled(): boolean {
    return this.isEnabled;
  }

  public speak(
    text: string,
    onStart?: () => void,
    onEnd?: () => void,
    onError?: () => void
  ) {
    if (!this.isEnabled || typeof window === 'undefined' || !('speechSynthesis' in window)) {
      if (onEnd) onEnd();
      return;
    }

    // Stop ongoing speech
    this.stop();

    // Clean text of markdown asterisks/emojis for clean speech
    const cleanText = text
      .replace(/[*_#`~]/g, '')
      .replace(/\/[\w]+/g, '')
      .replace(/[\u{1F600}-\u{1F64F}\u{1F300}-\u{1F5FF}\u{1F680}-\u{1F6FF}\u{2600}-\u{26FF}\u{2700}-\u{27BF}]/gu, '')
      .trim();

    if (!cleanText) {
      if (onEnd) onEnd();
      return;
    }

    try {
      const utter = new SpeechSynthesisUtterance(cleanText);
      if (this.voice) {
        utter.voice = this.voice;
      }
      utter.rate = 1.05;
      utter.pitch = 1.15; // Friendly energetic companion pitch

      utter.onstart = () => {
        if (onStart) onStart();
      };

      utter.onend = () => {
        this.currentUtterance = null;
        if (onEnd) onEnd();
      };

      utter.onerror = () => {
        this.currentUtterance = null;
        if (onError) onError();
        else if (onEnd) onEnd();
      };

      this.currentUtterance = utter;
      window.speechSynthesis.speak(utter);
    } catch {
      if (onEnd) onEnd();
    }
  }

  public stop() {
    if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
      window.speechSynthesis.cancel();
      this.currentUtterance = null;
    }
  }

  public isSpeaking(): boolean {
    if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
      return window.speechSynthesis.speaking;
    }
    return false;
  }
}

export const speechService = new SpeechService();
