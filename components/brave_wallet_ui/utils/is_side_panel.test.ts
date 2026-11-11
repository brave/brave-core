// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { IS_SIDE_PANEL_SESSION_KEY, resolveIsSidePanel } from './is_side_panel'

class MemoryStorage implements Storage {
  private store = new Map<string, string>()

  get length() {
    return this.store.size
  }

  clear() {
    this.store.clear()
  }

  getItem(key: string) {
    return this.store.has(key) ? this.store.get(key)! : null
  }

  key(index: number) {
    return Array.from(this.store.keys())[index] ?? null
  }

  removeItem(key: string) {
    this.store.delete(key)
  }

  setItem(key: string, value: string) {
    this.store.set(key, value)
  }
}

describe('resolveIsSidePanel', () => {
  it('returns true and persists when the query marker is present', () => {
    const storage = new MemoryStorage()

    expect(resolveIsSidePanel('?isSidePanel=true', storage)).toBe(true)
    expect(storage.getItem(IS_SIDE_PANEL_SESSION_KEY)).toBe('true')
  })

  it('returns true from session storage after the query is dropped', () => {
    const storage = new MemoryStorage()
    storage.setItem(IS_SIDE_PANEL_SESSION_KEY, 'true')

    expect(resolveIsSidePanel('', storage)).toBe(true)
    expect(resolveIsSidePanel('?foo=1', storage)).toBe(true)
  })

  it('returns false when neither query nor session marker is set', () => {
    const storage = new MemoryStorage()

    expect(resolveIsSidePanel('', storage)).toBe(false)
    expect(resolveIsSidePanel('?isSidePanel=false', storage)).toBe(false)
  })
})
