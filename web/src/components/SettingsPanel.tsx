// ============================================================
// B.A.N. — SettingsPanel.tsx
// Settings, personality display, and developer debug panel
// ============================================================

import { useState } from 'react';
import type { DeviceStatus, BanSettings } from '../types/device';

interface Props {
  status: DeviceStatus;
  settings: BanSettings;
  personality: { name: string; content: string } | null;
  onReloadPersonality: () => void;
  onDisconnect: () => void;
  onSettingsChange: (s: BanSettings) => void;
  lastRawPacket?: string;
}

export default function SettingsPanel({
  status,
  settings,
  personality,
  onReloadPersonality,
  onDisconnect,
  onSettingsChange,
  lastRawPacket,
}: Props) {
  const [showDebug, setShowDebug] = useState(false);

  const toggle = (key: keyof BanSettings) => {
    const val = settings[key];
    if (typeof val === 'boolean') {
      onSettingsChange({ ...settings, [key]: !val });
    }
  };

  return (
    <div style={{ flex: 1, overflowY: 'auto', padding: 'var(--space-lg)', display: 'flex', flexDirection: 'column', gap: 'var(--space-lg)' }}>

      {/* ── Personality ── */}
      <Section title="Personality">
        {personality ? (
          <>
            <InfoRow label="Name" value={personality.name} />
            <div style={{ marginTop: '8px' }}>
              <p className="text-sm text-muted" style={{ marginBottom: '6px' }}>Content Preview</p>
              <pre style={{
                fontSize: '12px',
                color: 'var(--text-secondary)',
                background: 'var(--glass-bg)',
                border: '1px solid var(--glass-border)',
                borderRadius: 'var(--radius-md)',
                padding: '10px 12px',
                whiteSpace: 'pre-wrap',
                fontFamily: 'var(--font-mono)',
                maxHeight: '180px',
                overflowY: 'auto',
              }}>
                {personality.content.slice(0, 600)}{personality.content.length > 600 ? '…' : ''}
              </pre>
            </div>
            <button
              className="btn btn-ghost"
              onClick={onReloadPersonality}
              id="btn-reload-personality"
              style={{ marginTop: '8px', fontSize: '12px' }}
            >
              ↻ Reload from ESP32
            </button>
          </>
        ) : (
          <p className="text-sm text-muted">Personality not yet loaded. Connect to R.A.N. first.</p>
        )}
      </Section>

      {/* ── Device Info ── */}
      <Section title="Device">
        <InfoRow label="Firmware" value={status.firmware ?? 'Unknown'} />
        <InfoRow label="Uptime"   value={status.uptime   ?? 'Unknown'} />
        <InfoRow label="Free Heap" value={status.freeHeap !== undefined ? `${Math.round(status.freeHeap / 1024)} KB` : 'Unknown'} />
        <InfoRow label="SD Total"  value={status.sdTotal  !== undefined ? `${Math.round(status.sdTotal  / 1024)} KB` : 'Unknown'} />
        <InfoRow label="SD Used"   value={status.sdUsed   !== undefined ? `${Math.round(status.sdUsed   / 1024)} KB` : 'Unknown'} />
        <InfoRow label="Memories"  value={status.memoryCount !== undefined ? `${status.memoryCount} records` : 'Unknown'} />
        <div style={{ marginTop: '12px' }}>
          <button
            className="btn btn-danger"
            onClick={onDisconnect}
            id="btn-disconnect-settings"
            style={{ fontSize: '12px' }}
          >
            Disconnect Device
          </button>
        </div>
      </Section>

      {/* ── Memory & Logging ── */}
      <Section title="Memory & Logging">
        <ToggleRow
          label="Automatic memory"
          description="Automatically save AI-suggested memories"
          checked={settings.autoMemory}
          onChange={() => toggle('autoMemory')}
          id="toggle-auto-memory"
        />
        <ToggleRow
          label="Conversation logging"
          description="Save conversations to microSD"
          checked={settings.conversationLogging}
          onChange={() => toggle('conversationLogging')}
          id="toggle-conv-logging"
        />
      </Section>

      {/* ── Interface ── */}
      <Section title="Interface">
        <ToggleRow
          label="Animations"
          description="Enable character and UI animations"
          checked={settings.animations}
          onChange={() => toggle('animations')}
          id="toggle-animations"
        />
        <ToggleRow
          label="Compact mode"
          description="Reduce padding and spacing"
          checked={settings.compactMode}
          onChange={() => toggle('compactMode')}
          id="toggle-compact"
        />
      </Section>

      {/* ── Developer ── */}
      <Section title="Developer">
        <button
          className="btn btn-ghost"
          onClick={() => setShowDebug(!showDebug)}
          aria-expanded={showDebug}
          id="btn-toggle-debug"
          style={{ fontSize: '12px' }}
        >
          {showDebug ? '▲ Hide Debug Panel' : '▼ Show Debug Panel'}
        </button>

        {showDebug && (
          <div style={{
            marginTop: '12px',
            padding: '12px',
            background: 'var(--glass-bg)',
            border: '1px solid var(--glass-border)',
            borderRadius: 'var(--radius-md)',
            display: 'flex',
            flexDirection: 'column',
            gap: '8px',
          }}>
            <InfoRow label="BLE Service" value="4fafc201-1fb5-459e-8fcc-c5c9c3319141" mono />
            <InfoRow label="TX UUID"     value="beb5483e-36e1-4688-b7f5-ea07361b26a8" mono />
            <InfoRow label="RX UUID"     value="6e400002-b5a3-f393-e0a9-e50e24dcca9e" mono />
            <InfoRow label="ESP32 Heap"  value={status.freeHeap !== undefined ? `${status.freeHeap} bytes` : 'N/A'} />
            <InfoRow label="SD Card"     value={status.sdCard ? 'OK' : 'FAIL'} />
            <InfoRow label="AI Backend"  value={status.aiBackend ? 'Available' : 'Unavailable'} />
            {lastRawPacket && (
              <div>
                <p className="text-sm text-muted" style={{ marginBottom: '4px' }}>Last Packet</p>
                <pre style={{ fontSize: '11px', fontFamily: 'var(--font-mono)', color: 'var(--text-secondary)', wordBreak: 'break-all', whiteSpace: 'pre-wrap' }}>
                  {lastRawPacket.slice(0, 300)}
                </pre>
              </div>
            )}
          </div>
        )}
      </Section>
    </div>
  );
}

// ── Helper sub-components ────────────────────────────────────

function Section({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <section>
      <h3 style={{ fontSize: '11px', fontWeight: 600, textTransform: 'uppercase', letterSpacing: '0.1em', color: 'var(--text-muted)', marginBottom: '12px' }}>
        {title}
      </h3>
      <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
        {children}
      </div>
    </section>
  );
}

function InfoRow({ label, value, mono }: { label: string; value: string; mono?: boolean }) {
  return (
    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', gap: '8px', flexWrap: 'wrap' }}>
      <span style={{ fontSize: '13px', color: 'var(--text-muted)' }}>{label}</span>
      <span style={{ fontSize: mono ? '11px' : '13px', fontFamily: mono ? 'var(--font-mono)' : undefined, color: 'var(--text-secondary)', wordBreak: 'break-all', textAlign: 'right', maxWidth: '60%' }}>
        {value}
      </span>
    </div>
  );
}

function ToggleRow({
  label, description, checked, onChange, id,
}: {
  label: string;
  description: string;
  checked: boolean;
  onChange: () => void;
  id: string;
}) {
  return (
    <label
      htmlFor={id}
      style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', gap: '12px', cursor: 'pointer' }}
    >
      <div>
        <p style={{ fontSize: '13px', color: 'var(--text-primary)', fontWeight: 500 }}>{label}</p>
        <p style={{ fontSize: '11px', color: 'var(--text-muted)', marginTop: '2px' }}>{description}</p>
      </div>
      <button
        id={id}
        role="switch"
        aria-checked={checked}
        onClick={onChange}
        style={{
          flexShrink: 0,
          width: '40px',
          height: '22px',
          borderRadius: '11px',
          border: `1px solid ${checked ? 'var(--accent)' : 'var(--glass-border)'}`,
          background: checked ? 'var(--accent)' : 'var(--glass-bg)',
          cursor: 'pointer',
          position: 'relative',
          transition: 'all 0.2s ease',
        }}
      >
        <span style={{
          position: 'absolute',
          top: '2px',
          left: checked ? '18px' : '2px',
          width: '16px',
          height: '16px',
          borderRadius: '50%',
          background: checked ? '#080c14' : 'rgba(255,255,255,0.3)',
          transition: 'left 0.2s ease',
        }} />
      </button>
    </label>
  );
}
