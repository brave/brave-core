/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StateStore } from '$web-common/state_store'

/**
 * Creates a React context provider component for managing application state.
 * The returned Provider component has `useState` and `useActions` hooks
 * attached as static methods for accessing state and actions within the
 * provider tree.
 *
 * @returns A Provider component with attached `useState` and `useActions`
 *   hooks.
 *
 * @example
 * // 1. Define state and actions:
 *
 * interface AppState {
 *   count: number
 *   name: string
 * }
 *
 * function defaultState(): AppState {
 *   return { count: 0, name: 'test' }
 * }
 *
 * interface AppActions {
 *   increment(): void
 *   setName(name: string): void
 * }
 *
 * function createAppState() {
 *   const store = createStateStore(defaultState())
 *   const actions: AppActions = {
 *     increment() {
 *       store.update((s) => ({ count: s.count + 1 }))
 *     },
 *     setName(name) {
 *       store.update({ name })
 *     },
 *   }
 *   return { store, actions }
 * }
 *
 * // 2. Create the provider:
 *
 * export const AppStateProvider = createStateProvider<AppState, AppActions>()
 *
 * // Export hooks for convenience:
 *
 * export const useAppState = AppStateProvider.useState
 * export const useAppActions = AppStateProvider.useActions
 *
 * // Wrap your app with the provider. The optional `name` prop exposes the
 * // store to `window.appState[name]` for debugging.
 *
 * function App() {
 *   return (
 *     <AppStateProvider name='myApp' value={createAppState()}>
 *       <MyComponent />
 *     </AppStateProvider>
 *   )
 * }
 *
 * // Use hooks in components to access state and actions:
 *
 * function MyComponent() {
 *   // Select specific state values with a mapping function.
 *   const count = useAppState((s) => s.count)
 *   const name = useAppState((s) => s.name)
 *
 *   // Get actions to update state.
 *   const actions = useAppActions()
 *
 *   return (
 *     <button onClick={() => actions.increment()}>
 *       {count}
 *     </button>
 *   )
 * }
 */
export function createStateProvider<State, Actions>() {
  interface ContextValue {
    store: StateStore<State>
    actions: Actions
  }

  const context = React.createContext<ContextValue | null>(null)

  function useContextValue(): ContextValue {
    const value = React.useContext(context)
    if (!value) {
      throw new Error('State context value has not been set')
    }
    return value
  }

  function useActions(): Actions {
    return useContextValue().actions
  }

  function useState<T>(map: (state: State) => T): T {
    const { store } = useContextValue()
    const [value, setValue] = React.useState(() => map(store.getState()))
    React.useEffect(() => {
      setValue(map(store.getState()))
      return store.addListener((state) => {
        setValue(map(state))
      })
    }, [store])
    return value
  }

  interface ProviderProps {
    name?: string
    value: ContextValue
    children: React.ReactNode
  }

  function Provider(props: ProviderProps) {
    const { name, value, children } = props

    React.useEffect(() => {
      if (name) {
        exposeState(name, value.store)
      }
    }, [name, value])

    return React.createElement(context.Provider, { value }, children)
  }

  Provider.useState = useState
  Provider.useActions = useActions

  return Provider
}

function exposeState(name: string, value: unknown) {
  let appState = Reflect.get(self, 'appState')
  if (typeof appState !== 'object') {
    appState = {}
    Reflect.set(self, 'appState', appState)
  }
  appState[name] = value
}
