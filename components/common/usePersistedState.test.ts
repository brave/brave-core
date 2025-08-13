/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as assert from 'assert'
import { renderHook, act } from '@testing-library/react'

import { usePersistedState, usePersistedJSON } from './usePersistedState'

describe('usePersistedState', () => {
  beforeEach(() => { localStorage.clear() })

  it('calls parse with an empty string if value is not present', () => {
    const { result } = renderHook(() => usePersistedState({
      key: 'local-storage-key',
      parse: (value) => value,
      stringify: (state) => state
    }))
    const [value] = result.current
    assert.equal(value, '')
  })

  it('calls parse with the current local storage string value', () => {
    localStorage.setItem('local-storage-key', 'stored-value')
    const { result } = renderHook(() => usePersistedState({
      key: 'local-storage-key',
      parse: (value) => value,
      stringify: (state) => state
    }))
    const [value] = result.current
    assert.equal(value, 'stored-value')
  })

  it('saves the value to localStorage when updated', () => {
    const { result } = renderHook(() => usePersistedState({
      key: 'local-storage-key',
      parse: (value) => Number(value) || 0,
      stringify: (state) => String(state)
    }))
    const [, setValue] = result.current
    act(() => { setValue(100) })
    assert.equal(localStorage.getItem('local-storage-key'), '100')
  })
})

describe('usePersistedJSON', () => {
  beforeEach(() => { localStorage.clear() })

  it('calls mapData with null if value is not present', () => {
    const { result } = renderHook(() => usePersistedJSON(
      'local-storage-key',
      (data) => data === null
    ))
    const [value] = result.current
    assert.equal(value, true)
  })

  it('calls mapData with null if value is not JSON', () => {
    localStorage.setItem('local-storage-key', '[')
    const { result } = renderHook(() => usePersistedJSON(
      'local-storage-key',
      (data) => data === null
    ))
    const [value] = result.current
    assert.equal(value, true)
  })

  it('calls mapData with the current local storage string value', () => {
    localStorage.setItem('local-storage-key', '{"key": "value", "number": 42}')
    const { result } = renderHook(() => usePersistedJSON(
      'local-storage-key',
      (data) => data
    ))
    const [value] = result.current
    assert.deepEqual(value, { key: 'value', number: 42 })
  })

  it('saves the value to localStorage when updated', () => {
    const { result } = renderHook(() => usePersistedJSON(
      'local-storage-key',
      (data) => String(data ?? '')
    ))
    const [, setValue] = result.current
    act(() => { setValue('hello') })
    assert.equal(localStorage.getItem('local-storage-key'), '"hello"')
  })
})
