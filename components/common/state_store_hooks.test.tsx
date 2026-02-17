/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { act, renderHook } from '@testing-library/react'

import { createStateStore, StateStore } from './state_store'
import { createUseStateHook } from './state_store_hooks'

interface TestState {
  count: number
  name: string
}

const TestStoreContext = React.createContext<StateStore<TestState> | null>(null)
const useTestState = createUseStateHook(TestStoreContext)

function createWrapper(store: StateStore<TestState>) {
  return function Wrapper({ children }: { children: React.ReactNode }) {
    return (
      <TestStoreContext.Provider value={store}>
        {children}
      </TestStoreContext.Provider>
    )
  }
}

describe('createUseStateHook', () => {
  it('throws an error when used outside of a provider', () => {
    expect(() => {
      renderHook(() => useTestState((s) => s.count))
    }).toThrow('useStateStore must be used within a StateStore Provider')
  })

  it('returns the initial mapped state', () => {
    const store = createStateStore<TestState>({ count: 0, name: 'test' })
    const { result } = renderHook(() => useTestState((s) => s.count), {
      wrapper: createWrapper(store),
    })
    expect(result.current).toBe(0)
  })

  it('returns a derived value from state', () => {
    const store = createStateStore<TestState>({ count: 5, name: 'hello' })
    const { result } = renderHook(
      () => useTestState((s) => `${s.name}: ${s.count}`),
      { wrapper: createWrapper(store) },
    )
    expect(result.current).toBe('hello: 5')
  })

  it('updates when selected state changes', async () => {
    const store = createStateStore<TestState>({ count: 0, name: 'test' })
    const { result } = renderHook(() => useTestState((s) => s.count), {
      wrapper: createWrapper(store),
    })

    expect(result.current).toBe(0)

    await act(async () => {
      store.update({ count: 10 })
      await null // Wait for microtask to flush
    })

    expect(result.current).toBe(10)
  })

  it('updates when selecting a different part of state', async () => {
    const store = createStateStore<TestState>({ count: 0, name: 'initial' })
    const { result } = renderHook(() => useTestState((s) => s.name), {
      wrapper: createWrapper(store),
    })

    expect(result.current).toBe('initial')

    await act(async () => {
      store.update({ name: 'updated' })
      await null
    })

    expect(result.current).toBe('updated')
  })

  it('does not cause unnecessary re-renders', async () => {
    const store = createStateStore<TestState>({ count: 0, name: 'test' })
    let renderCount = 0

    const { result } = renderHook(
      () => {
        renderCount++
        return useTestState((s) => s.count)
      },
      { wrapper: createWrapper(store) },
    )

    expect(result.current).toBe(0)
    expect(renderCount).toBe(1)

    // Update a different part of state
    await act(async () => {
      store.update({ name: 'changed' })
      await null
    })

    // The hook still re-renders because it receives all state updates,
    // but the component can optimize with React.memo if needed
    expect(result.current).toBe(0)
    expect(renderCount).toBe(1)
  })

  it('handles multiple rapid updates', async () => {
    const store = createStateStore<TestState>({ count: 0, name: 'test' })
    const { result } = renderHook(() => useTestState((s) => s.count), {
      wrapper: createWrapper(store),
    })

    await act(async () => {
      store.update({ count: 1 })
      store.update({ count: 2 })
      store.update({ count: 3 })
      await null
    })

    expect(result.current).toBe(3)
  })

  it('cleans up listener on unmount', async () => {
    const store = createStateStore<TestState>({ count: 0, name: 'test' })
    const { result, unmount } = renderHook(() => useTestState((s) => s.count), {
      wrapper: createWrapper(store),
    })

    expect(result.current).toBe(0)

    unmount()

    // Update after unmount should not cause issues
    await act(async () => {
      store.update({ count: 100 })
      await null
    })
  })

  it('works with complex selector functions', async () => {
    interface ComplexState {
      items: { id: number; value: string }[]
      selectedId: number | null
    }

    const ComplexContext =
      React.createContext<StateStore<ComplexState> | null>(null)
    const useComplexState = createUseStateHook(ComplexContext)

    const store = createStateStore<ComplexState>({
      items: [
        { id: 1, value: 'first' },
        { id: 2, value: 'second' },
      ],
      selectedId: 1,
    })

    function wrapper({ children }: { children: React.ReactNode }) {
      return (
        <ComplexContext.Provider value={store}>
          {children}
        </ComplexContext.Provider>
      )
    }

    const { result } = renderHook(
      () =>
        useComplexState((s) => {
          const selected = s.items.find((item) => item.id === s.selectedId)
          return selected?.value ?? 'none'
        }),
      { wrapper },
    )

    expect(result.current).toBe('first')

    await act(async () => {
      store.update({ selectedId: 2 })
      await null
    })

    expect(result.current).toBe('second')

    await act(async () => {
      store.update({ selectedId: null })
      await null
    })

    expect(result.current).toBe('none')
  })

  it('re-syncs state when store reference changes', async () => {
    const store1 = createStateStore<TestState>({ count: 1, name: 'store1' })
    const store2 = createStateStore<TestState>({ count: 2, name: 'store2' })

    let currentStore = store1

    function DynamicWrapper({ children }: { children: React.ReactNode }) {
      return (
        <TestStoreContext.Provider value={currentStore}>
          {children}
        </TestStoreContext.Provider>
      )
    }

    const { result, rerender } = renderHook(() =>
      useTestState((s) => s.count),
      {
        wrapper: DynamicWrapper,
      },
    )

    expect(result.current).toBe(1)

    // Change the store reference
    currentStore = store2
    rerender()

    expect(result.current).toBe(2)
  })
})
