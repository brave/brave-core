/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as assert from 'assert'

import { createStateStore } from './state_store'

describe('createStateStore', () => {
  it('returns initial state from getState', () => {
    const store = createStateStore({ count: 0, name: 'test' })
    assert.deepEqual(store.getState(), { count: 0, name: 'test' })
  })

  it('updates state with a partial object', async () => {
    const store = createStateStore({ count: 0, name: 'test' })
    store.update({ count: 5 })
    assert.deepEqual(store.getState(), { count: 5, name: 'test' })
  })

  it('updates state with an update function', async () => {
    const store = createStateStore({ count: 0 })
    store.update((state) => ({ count: state.count + 1 }))
    assert.equal(store.getState().count, 1)
  })

  it('ignores undefined values in updates', () => {
    const store = createStateStore({ count: 0, name: 'test' })
    store.update({ count: undefined, name: 'updated' })
    assert.deepEqual(store.getState(), { count: 0, name: 'updated' })
  })

  it('notifies listeners after state changes', async () => {
    const store = createStateStore({ count: 0 })
    const states: { count: number }[] = []
    store.addListener((state) => states.push({ ...state }))

    store.update({ count: 1 })
    assert.equal(states.length, 0)

    await null
    assert.equal(states.length, 1)
    assert.deepEqual(states[0], { count: 1 })
  })

  it('does not notify listeners when state has not changed', async () => {
    const store = createStateStore({ count: 0 })
    let callCount = 0
    store.addListener(() => callCount++)

    store.update({ count: 0 })
    await null
    assert.equal(callCount, 0)
  })

  it('batches multiple updates into a single notification', async () => {
    const store = createStateStore({ count: 0 })
    let callCount = 0
    store.addListener(() => callCount++)

    store.update({ count: 1 })
    store.update({ count: 2 })
    store.update({ count: 3 })

    await null
    assert.equal(callCount, 1)
    assert.equal(store.getState().count, 3)
  })

  it('removes listener when unsubscribe function is called', async () => {
    const store = createStateStore({ count: 0 })
    let callCount = 0
    const unsubscribe = store.addListener(() => callCount++)

    store.update({ count: 1 })
    await null
    assert.equal(callCount, 1)

    unsubscribe()

    store.update({ count: 2 })
    await null
    assert.equal(callCount, 1)
  })

  it('does not add duplicate listeners', async () => {
    const store = createStateStore({ count: 0 })
    let callCount = 0
    const listener = () => callCount++

    store.addListener(listener)
    store.addListener(listener)

    store.update({ count: 1 })
    await null
    assert.equal(callCount, 1)
  })

  it('exits notification loop if listener triggers an update', async () => {
    const store = createStateStore({ count: 0 })
    const callOrder: string[] = []

    store.addListener((state) => {
      callOrder.push(`first:${state.count}`)
      if (state.count === 1) {
        store.update({ count: 2 })
      }
    })

    store.addListener((state) => {
      callOrder.push(`second:${state.count}`)
    })

    store.update({ count: 1 })
    await null

    assert.deepEqual(callOrder, ['first:1'])
    await null
    assert.deepEqual(callOrder, ['first:1', 'first:2', 'second:2'])
  })
})
