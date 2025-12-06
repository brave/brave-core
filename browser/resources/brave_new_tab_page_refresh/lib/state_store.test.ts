/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStateStore } from './state_store'

interface TestState {
  count: number
  name: string
}

function flushMicrotasks() {
  return new Promise<void>((resolve) => queueMicrotask(resolve))
}

describe('createStateStore', () => {
  describe('getState', () => {
    it('returns the initial state', () => {
      const store = createStateStore({ count: 0, name: 'test' })
      expect(store.getState()).toEqual({ count: 0, name: 'test' })
    })

    it('returns updated state after update', async () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      store.update({ count: 1 })
      expect(store.getState()).toEqual({ count: 1, name: 'test' })
    })
  })

  describe('update', () => {
    it('updates state with partial object', () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      store.update({ count: 5 })
      expect(store.getState().count).toBe(5)
      expect(store.getState().name).toBe('test')
    })

    it('updates state with update function', () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      store.update((state) => ({ count: state.count + 1 }))
      expect(store.getState().count).toBe(1)
    })

    it('ignores undefined values in partial update', () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      store.update({ count: undefined, name: 'updated' })
      expect(store.getState().count).toBe(0)
      expect(store.getState().name).toBe('updated')
    })

    it('does not notify listeners when value is unchanged', async () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      const listener = jest.fn()
      store.addListener(listener)

      store.update({ count: 0 })
      await flushMicrotasks()

      expect(listener).not.toHaveBeenCalled()
    })

    it('does not notify listeners when all values are undefined', async () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      const listener = jest.fn()
      store.addListener(listener)

      store.update({ count: undefined, name: undefined })
      await flushMicrotasks()

      expect(listener).not.toHaveBeenCalled()
    })
  })

  describe('addListener', () => {
    it('listener is not called immediately when added', () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      const listener = jest.fn()
      store.addListener(listener)
      expect(listener).not.toHaveBeenCalled()
    })

    it('listener is called asynchronously after update', async () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      const listener = jest.fn()
      store.addListener(listener)

      store.update({ count: 1 })
      expect(listener).not.toHaveBeenCalled()

      await flushMicrotasks()
      expect(listener).toHaveBeenCalledWith({ count: 1, name: 'test' })
    })

    it('returns a function to remove the listener', async () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      const listener = jest.fn()
      const unsubscribe = store.addListener(listener)

      unsubscribe()
      store.update({ count: 1 })
      await flushMicrotasks()

      expect(listener).not.toHaveBeenCalled()
    })

    it('does not add duplicate listeners', async () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      const listener = jest.fn()

      store.addListener(listener)
      store.addListener(listener)
      store.update({ count: 1 })
      await flushMicrotasks()

      expect(listener).toHaveBeenCalledTimes(1)
    })
  })

  describe('notification batching', () => {
    it('multiple rapid updates result in one notification', async () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      const listener = jest.fn()
      store.addListener(listener)

      store.update({ count: 1 })
      store.update({ count: 2 })
      store.update({ count: 3 })
      await flushMicrotasks()

      expect(listener).toHaveBeenCalledTimes(1)
      expect(listener).toHaveBeenCalledWith({ count: 3, name: 'test' })
    })

    it('update during notification exits notification loop', async () => {
      const store = createStateStore<TestState>({ count: 0, name: 'test' })
      const calls: number[] = []

      const listener1 = jest.fn(() => {
        calls.push(1)
        if (store.getState().count === 1) {
          store.update({ count: 2 })
        }
      })
      const listener2 = jest.fn(() => {
        calls.push(2)
      })

      store.addListener(listener1)
      store.addListener(listener2)
      store.update({ count: 1 })

      await flushMicrotasks()
      expect(calls).toEqual([1, 1, 2])
    })
  })
})
