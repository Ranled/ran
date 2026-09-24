// ============================================================
// B.A.N. — MemoryPanel.tsx
// View, search, add, and delete memory records.
// ============================================================

import { useState } from 'react';
import './MemoryPanel.css';
import type { MemoryRecord, MemoryCategory } from '../types/memory';

const CATEGORIES: MemoryCategory[] = [
  'PROJECT', 'PREFERENCE', 'PERSONAL', 'KNOWLEDGE',
  'DEVICE', 'TASK', 'NOTE', 'CONVERSATION',
];

interface Props {
  memories: MemoryRecord[];
  onSave: (category: MemoryCategory, key: string, value: string) => void;
  onDelete: (id: number) => void;
  onSearch: (keyword: string) => void;
  onRefresh: () => void;
  isLoading: boolean;
}

export default function MemoryPanel({
  memories,
  onSave,
  onDelete,
  onSearch,
  onRefresh,
  isLoading,
}: Props) {
  const [searchText, setSearchText] = useState('');
  const [showAddForm, setShowAddForm] = useState(false);
  const [newCategory, setNewCategory] = useState<MemoryCategory>('NOTE');
  const [newKey, setNewKey] = useState('');
  const [newValue, setNewValue] = useState('');
  const [deleteConfirm, setDeleteConfirm] = useState<number | null>(null);

  const handleSearch = (val: string) => {
    setSearchText(val);
    if (val.trim()) {
      onSearch(val.trim());
    }
  };

  const handleSave = () => {
    if (!newKey.trim() || !newValue.trim()) return;
    onSave(newCategory, newKey.trim(), newValue.trim());
    setNewKey('');
    setNewValue('');
    setShowAddForm(false);
  };

  const handleDeleteClick = (id: number) => {
    if (deleteConfirm === id) {
      onDelete(id);
      setDeleteConfirm(null);
    } else {
      setDeleteConfirm(id);
      // Auto-cancel after 3s
      setTimeout(() => setDeleteConfirm(null), 3000);
    }
  };

  const filteredMemories = searchText
    ? memories.filter(m =>
        m.key.toLowerCase().includes(searchText.toLowerCase()) ||
        m.value.toLowerCase().includes(searchText.toLowerCase()) ||
        m.category.toLowerCase().includes(searchText.toLowerCase())
      )
    : memories;

  return (
    <div className="memory-panel">
      {/* Header */}
      <div className="memory-header">
        <h2 className="memory-header-title">Memory</h2>
        <div style={{ display: 'flex', gap: '8px', alignItems: 'center' }}>
          <span className="text-sm text-muted">{memories.length} records</span>
          <button
            className="btn btn-ghost"
            onClick={onRefresh}
            disabled={isLoading}
            aria-label="Refresh memory list"
            id="btn-memory-refresh"
            style={{ padding: '5px 10px', fontSize: '12px' }}
          >
            {isLoading ? '…' : '↻ Refresh'}
          </button>
        </div>
      </div>

      {/* Search */}
      <div className="memory-search-bar">
        <input
          type="search"
          className="input memory-search-input"
          placeholder="Search memories…"
          value={searchText}
          onChange={e => handleSearch(e.target.value)}
          aria-label="Search memories"
          id="input-memory-search"
        />
      </div>

      {/* Memory list */}
      <div className="memory-list" aria-label="Memory records">
        {isLoading && (
          <div className="empty-state">
            <p className="text-sm text-muted">Loading memories…</p>
          </div>
        )}

        {!isLoading && filteredMemories.length === 0 && (
          <div className="empty-state">
            <span className="empty-state-icon">🧠</span>
            <p className="text-sm text-muted">
              {searchText ? `No memories matching "${searchText}"` : 'No memories stored yet.'}
            </p>
          </div>
        )}

        {filteredMemories.map(mem => (
          <article key={mem.id} className="memory-card">
            <div className="memory-card-body">
              <div className="memory-card-header">
                <span className="badge badge-category">{mem.category}</span>
                <span className="memory-card-key">{mem.key}</span>
              </div>
              <p className="memory-card-value">{mem.value}</p>
              <p className="memory-card-meta">#{mem.id} · {mem.created}</p>
            </div>
            <div className="memory-card-actions">
              <button
                className={`btn ${deleteConfirm === mem.id ? 'btn-danger' : 'btn-icon'}`}
                onClick={() => handleDeleteClick(mem.id)}
                aria-label={deleteConfirm === mem.id ? 'Confirm delete' : `Delete memory ${mem.id}`}
                id={`btn-delete-memory-${mem.id}`}
                style={{ fontSize: '12px', padding: deleteConfirm === mem.id ? '4px 8px' : '6px' }}
              >
                {deleteConfirm === mem.id ? 'Confirm?' : '✕'}
              </button>
            </div>
          </article>
        ))}
      </div>

      {/* Add form toggle */}
      <div className="memory-add-toggle">
        <button
          className="btn btn-ghost"
          onClick={() => setShowAddForm(!showAddForm)}
          aria-expanded={showAddForm}
          id="btn-memory-add-toggle"
          style={{ fontSize: '13px' }}
        >
          {showAddForm ? '✕ Cancel' : '+ Add Memory'}
        </button>
      </div>

      {/* Add form */}
      {showAddForm && (
        <div className="memory-add-form">
          <div className="form-row">
            <div className="form-field">
              <label className="form-label" htmlFor="sel-category">Category</label>
              <select
                id="sel-category"
                className="input select"
                value={newCategory}
                onChange={e => setNewCategory(e.target.value as MemoryCategory)}
              >
                {CATEGORIES.map(c => (
                  <option key={c} value={c}>{c}</option>
                ))}
              </select>
            </div>
            <div className="form-field">
              <label className="form-label" htmlFor="input-mem-key">Key / Name</label>
              <input
                id="input-mem-key"
                type="text"
                className="input"
                placeholder="e.g. CHARRMPASS"
                value={newKey}
                onChange={e => setNewKey(e.target.value)}
              />
            </div>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="input-mem-value">Information</label>
            <textarea
              id="input-mem-value"
              className="input"
              placeholder="e.g. RFID vehicle monitoring project"
              value={newValue}
              onChange={e => setNewValue(e.target.value)}
              rows={3}
              style={{ resize: 'vertical' }}
            />
          </div>
          <button
            className="btn btn-primary"
            onClick={handleSave}
            disabled={!newKey.trim() || !newValue.trim()}
            id="btn-memory-save"
            style={{ alignSelf: 'flex-end' }}
          >
            Save Memory
          </button>
        </div>
      )}
    </div>
  );
}
