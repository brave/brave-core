/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render, screen, act } from '@testing-library/react'

import { createStateProvider } from './state_provider'
import { createStateStore } from '$web-common/state_store'

interface TestState {
  count: number
  name: string
}

function defaultState(): TestState {
  return { count: 0, name: 'test' }
}

interface TestActions {
  increment(): void
  setName(name: string): void
}

function createTestState() {
  const store = createStateStore(defaultState())
  const actions: TestActions = {
    increment() {
      store.update((s) => ({ count: s.count + 1 }))
    },
    setName(name: string) {
      store.update({ name })
    },
  }
  return { store, actions }
}

function flushMicrotasks() {
  return act(() => new Promise<void>((resolve) => queueMicrotask(resolve)))
}

describe('createStateProvider', () => {
  describe('Provider', () => {
    it('renders children', () => {
      const TestProvider = createStateProvider<TestState, TestActions>()
      render(
        <TestProvider value={createTestState()}>
          <div data-testid='child'>Hello</div>
        </TestProvider>,
      )
      expect(screen.getByTestId('child')).toHaveTextContent('Hello')
    })

    it('exposes store to window.appState when name prop is provided', () => {
      const TestProvider = createStateProvider<TestState, TestActions>()
      render(
        <TestProvider
          name='test'
          value={createTestState()}
        >
          <div />
        </TestProvider>,
      )
      expect((self as any).appState.test).toBeDefined()
      expect((self as any).appState.test.getState()).toEqual(defaultState())
    })
  })

  describe('useState', () => {
    it('returns the mapped state value', () => {
      const TestProvider = createStateProvider<TestState, TestActions>()

      function TestComponent() {
        const count = TestProvider.useState((s) => s.count)
        return <div data-testid='count'>{count}</div>
      }

      render(
        <TestProvider value={createTestState()}>
          <TestComponent />
        </TestProvider>,
      )
      expect(screen.getByTestId('count')).toHaveTextContent('0')
    })

    it('updates when state changes', async () => {
      const TestProvider = createStateProvider<TestState, TestActions>()

      function TestComponent() {
        const count = TestProvider.useState((s) => s.count)
        const actions = TestProvider.useActions()
        return (
          <button
            data-testid='button'
            onClick={() => actions.increment()}
          >
            {count}
          </button>
        )
      }

      render(
        <TestProvider value={createTestState()}>
          <TestComponent />
        </TestProvider>,
      )

      expect(screen.getByTestId('button')).toHaveTextContent('0')
      screen.getByTestId('button').click()
      await flushMicrotasks()
      expect(screen.getByTestId('button')).toHaveTextContent('1')
    })

    it('throws when used outside provider', () => {
      const TestProvider = createStateProvider<TestState, TestActions>()

      function TestComponent() {
        TestProvider.useState((s) => s.count)
        return null
      }

      expect(() => render(<TestComponent />)).toThrow(
        'State context value has not been set',
      )
    })
  })

  describe('useActions', () => {
    it('returns the actions object', () => {
      const TestProvider = createStateProvider<TestState, TestActions>()
      let capturedActions: TestActions | null = null

      function TestComponent() {
        capturedActions = TestProvider.useActions()
        return null
      }

      render(
        <TestProvider value={createTestState()}>
          <TestComponent />
        </TestProvider>,
      )

      expect(capturedActions).not.toBeNull()
      expect(typeof capturedActions!.increment).toBe('function')
      expect(typeof capturedActions!.setName).toBe('function')
    })

    it('throws when used outside provider', () => {
      const TestProvider = createStateProvider<TestState, TestActions>()

      function TestComponent() {
        TestProvider.useActions()
        return null
      }

      expect(() => render(<TestComponent />)).toThrow(
        'State context value has not been set',
      )
    })
  })
})
