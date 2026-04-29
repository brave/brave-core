// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Observer interface for memory changes.
 */
export interface MemoryStoreObserver {
  onMemoriesChanged(memories: string[]): void
}

/**
 * MemoryStore holds user memories and notifies observers of changes.
 * This is shared across UIHandler (for UI operations) and the engine
 * (for including memories in API requests).
 */
export default class MemoryStore {
  private memories: Set<string> = new Set()
  private observers: Set<MemoryStoreObserver> = new Set()

  constructor(initialMemories?: string[]) {
    if (initialMemories) {
      this.memories = new Set(initialMemories)
    }
  }

  // Add an observer
  addObserver(observer: MemoryStoreObserver): void {
    this.observers.add(observer)
  }

  // Remove an observer
  removeObserver(observer: MemoryStoreObserver): void {
    this.observers.delete(observer)
  }

  // Notify all observers of changes
  private notifyObservers(): void {
    const memoriesArray = this.getMemories()
    for (const observer of this.observers) {
      observer.onMemoriesChanged(memoriesArray)
    }
  }

  // Get all memories as an array
  getMemories(): string[] {
    return Array.from(this.memories)
  }

  // Check if a memory exists
  hasMemory(memory: string): boolean {
    return this.memories.has(memory)
  }

  // Add a memory
  addMemory(memory: string): void {
    if (!this.memories.has(memory)) {
      this.memories.add(memory)
      this.notifyObservers()
    }
  }

  // Delete a memory
  deleteMemory(memory: string): void {
    if (this.memories.delete(memory)) {
      this.notifyObservers()
    }
  }

  // Set all memories (replaces existing)
  setMemories(memories: string[]): void {
    this.memories = new Set(memories)
    this.notifyObservers()
  }

  // Clear all memories
  clearMemories(): void {
    if (this.memories.size > 0) {
      this.memories.clear()
      this.notifyObservers()
    }
  }

  /**
   * Get memories as a record for the engine API.
   * Returns null if there are no memories.
   */
  async getMemoryForEngine(): Promise<Record<string, unknown> | null> {
    if (this.memories.size === 0) {
      return null
    }

    // Return memories in the format expected by the engine
    return {
      memories: this.getMemories(),
    }
  }
}
