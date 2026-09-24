// ============================================================
// B.A.N. — types/memory.ts
// Memory record types
// ============================================================

export type MemoryCategory =
  | 'PROJECT'
  | 'PREFERENCE'
  | 'PERSONAL'
  | 'KNOWLEDGE'
  | 'DEVICE'
  | 'TASK'
  | 'NOTE'
  | 'CONVERSATION';

export interface MemoryRecord {
  id: number;
  category: MemoryCategory;
  key: string;
  value: string;
  created: string;
  type?: MemoryCategory;
  name?: string;
  content?: string;
  keywords?: string[];
}

export interface MemoryDatabase {
  memories: MemoryRecord[];
}
