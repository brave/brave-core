/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StateStore, createStateStore } from './state_store'

/**
 * Creates a React context provider component for managing application state.
 * The returned Provider component has `useState` and `useActions` hooks
 * attached as static methods for accessing state and actions within the
 * provider tree.
 *
 * @param initialState - The initial state object for the store.
 * @param createHandler - A function that receives the state store and returns
 *   an action handler. Action handlers typically call `store.update()` to
 *   modify state.
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
 * function createHandler(store: StateStore<AppState>): AppActions {
 *   return {
 *     increment() {
 *       store.update((s) => ({ count: s.count + 1 }))
 *     },
 *     setName(name) {
 *       store.update({ name })
 *     },
 *   }
 * }
 *
 * // 2. Create the provider:
 *
 * export const AppStateProvider = createStateProvider(
 *   defaultState(),
 *   createHandler
 * )
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
 *     <AppStateProvider name='myApp'>
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
export function createStateProvider<State, Actions>(
  initialState: State,
  createHandler: (store: StateStore<State>) => Actions,
) {
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
    createHandler?: (store: StateStore<State>) => Actions
    children: React.ReactNode
  }

  function Provider(props: ProviderProps) {
    const value = React.useMemo(() => {
      const store = createStateStore(initialState)
      const actions = (props.createHandler ?? createHandler)(store)
      return { store, actions }
    }, [props.createHandler])

    React.useEffect(() => {
      if (props.name) {
        exposeState(props.name, value.store)
      }
    }, [props.name, value])

    return React.createElement(context.Provider, { value }, props.children)
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
