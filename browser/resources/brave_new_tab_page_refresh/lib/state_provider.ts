/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Store, createStore } from './store'

export function createStateProvider<State, Actions>(
  initialState: State,
  createHandler: (store: Store<State>) => Actions,
) {
  interface ContextValue {
    store: Store<State>
    handler: Actions
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
    return useContextValue().handler
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
    createHandler?: (store: Store<State>) => Actions
    children: React.ReactNode
  }

  function Provider(props: ProviderProps) {
    const value = React.useMemo(() => {
      const store = createStore(initialState)
      const handler = (props.createHandler ?? createHandler)(store)
      return { store, handler }
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
