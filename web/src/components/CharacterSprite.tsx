// ============================================================
// B.A.N. — CharacterSprite.tsx
// Frame-by-frame animated sprite component for B.A.N.
// Renders pixel-art animations using the /character folder
// ============================================================

import { useState, useEffect, useRef } from 'react';
import type { CharacterMood } from '../types/device';
import './CharacterSprite.css';

export interface MoodConfig {
  name: string;
  label: string;
  icon: string;
  fps: number;
  frames: string[];
}

export const MOOD_DEFINITIONS: Record<CharacterMood, MoodConfig> = {
  idle: {
    name: 'idle',
    label: 'Idle',
    icon: '✨',
    fps: 4, // 250ms per frame
    frames: [
      '/character/IDLE 1.png',
      '/character/IDLE 2.png',
      '/character/IDLE 3.png',
      '/character/IDLE 4.png',
    ],
  },
  wave: {
    name: 'wave',
    label: 'Wave',
    icon: '👋',
    fps: 5, // 200ms
    frames: [
      '/character/WAVE 1.png',
      '/character/WAVE 2.png',
      '/character/WAVE 3.png',
      '/character/WAVE 4.png',
    ],
  },
  talking: {
    name: 'talking',
    label: 'Talking',
    icon: '💬',
    fps: 6, // 166ms
    frames: [
      '/character/TALKING 1.png',
      '/character/TALKING 2.png',
      '/character/TALKING 3.png',
      '/character/TALKING 4.png',
    ],
  },
  happy: {
    name: 'happy',
    label: 'Happy',
    icon: '😄',
    fps: 4, // 250ms
    frames: [
      '/character/HAPPY 1.png',
      '/character/HAPPY 2.png',
      '/character/HAPPY 3.png',
      '/character/HAPPY 4.png',
    ],
  },
  wondering: {
    name: 'wondering',
    label: 'Thinking',
    icon: '🤔',
    fps: 3, // 330ms
    frames: [
      '/character/WONDERING 1.png',
      '/character/WONDERING 2.png',
      '/character/WONDERING 3.png',
    ],
  },
  shock: {
    name: 'shock',
    label: 'Shock',
    icon: '⚡',
    fps: 5,
    frames: [
      '/character/SHOCK 1.png',
      '/character/SHOCK 2.png',
      '/character/SHOCK 3.png',
    ],
  },
  angry: {
    name: 'angry',
    label: 'Angry',
    icon: '💢',
    fps: 4,
    frames: [
      '/character/ANGRY 1.png',
      '/character/ANGRY 2.png',
      '/character/ANGRY 3.png',
    ],
  },
  sad: {
    name: 'sad',
    label: 'Sad',
    icon: '💧',
    fps: 2.5,
    frames: [
      '/character/SAD 1.png',
      '/character/SAD 2.png',
    ],
  },
  walking: {
    name: 'walking',
    label: 'Walking',
    icon: '🚶',
    fps: 7,
    frames: [
      '/character/WALKING 1.png',
      '/character/WALKING 2.png',
      '/character/WALKING 3.png',
      '/character/WALKING 4.png',
      '/character/WALKING 5.png',
      '/character/WALKING 6.png',
      '/character/WALKING 7.png',
    ],
  },
  running: {
    name: 'running',
    label: 'Running',
    icon: '🏃',
    fps: 9,
    frames: [
      '/character/RUNNING 1.png',
      '/character/RUNNING 2.png',
      '/character/RUNNING 3.png',
      '/character/RUNNING 4.png',
      '/character/RUNNING 5.png',
      '/character/RUNNING 6.png',
      '/character/RUNNING 7.png',
    ],
  },
};

interface Props {
  mood: CharacterMood;
  onClick?: () => void;
  bubbleText?: string | null;
  size?: 'compact' | 'standard' | 'large';
  interactive?: boolean;
}

export default function CharacterSprite({
  mood,
  onClick,
  bubbleText,
  size = 'standard',
  interactive = true,
}: Props) {
  const [frameIndex, setFrameIndex] = useState(0);
  const [isPreloaded, setIsPreloaded] = useState(false);
  const activeMoodDef = MOOD_DEFINITIONS[mood] || MOOD_DEFINITIONS.idle;

  // Preload all mood frames once on mount to prevent lag / flickering
  useEffect(() => {
    const urlsToPreload: string[] = [];
    Object.values(MOOD_DEFINITIONS).forEach(def => {
      def.frames.forEach(f => urlsToPreload.push(f));
    });

    let loadedCount = 0;
    urlsToPreload.forEach(src => {
      const img = new Image();
      img.src = src;
      img.onload = () => {
        loadedCount++;
        if (loadedCount >= urlsToPreload.length * 0.5) {
          setIsPreloaded(true);
        }
      };
    });
  }, []);

  // Frame animation loop
  useEffect(() => {
    setFrameIndex(0);
    const intervalMs = Math.round(1000 / activeMoodDef.fps);
    const totalFrames = activeMoodDef.frames.length;

    const timer = setInterval(() => {
      setFrameIndex(prev => (prev + 1) % totalFrames);
    }, intervalMs);

    return () => clearInterval(timer);
  }, [mood, activeMoodDef]);

  const currentSrc = activeMoodDef.frames[frameIndex] || activeMoodDef.frames[0];

  return (
    <div
      className={`character-sprite-stage size-${size} mood-${mood} ${interactive ? 'is-interactive' : ''}`}
      onClick={onClick}
      role={interactive ? 'button' : 'img'}
      tabIndex={interactive ? 0 : undefined}
      aria-label={`R.A.N. Character — ${activeMoodDef.label} mood`}
    >
      {/* Speech / Reaction Bubble */}
      {bubbleText && (
        <div className="character-speech-bubble" role="status">
          <span className="speech-text">{bubbleText}</span>
          <div className="speech-arrow" />
        </div>
      )}

      {/* Halo / Aura Glow */}
      <div className={`character-aura aura-${mood}`} aria-hidden="true" />

      {/* Pixel Art Sprite Frame */}
      <div className="character-frame-wrapper">
        <img
          key={mood}
          src={currentSrc}
          alt={`R.A.N. ${activeMoodDef.label}`}
          className="character-sprite-img"
          draggable={false}
        />
      </div>

      {/* Ground Pedestal Shadow */}
      <div className="pedestal-contact-shadow" aria-hidden="true" />
    </div>
  );
}
