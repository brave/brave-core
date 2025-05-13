/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// An "API" is an interface that encapsulates a set of actions and a current
// state, and will generally wrap a connection to a service that is external to
// the application (e.g. a browser service provided via a mojo interface, or a
// WebUI page handler interface). The state may be accessed at any time, and
// listeners can be notified when the state has changed.
export type API<State, Actions> = Actions & {
  getState: () => State,
  addListener: (listener: (state: State) => void) => void
}

// Creates a React context and associated hooks for an API.
export function createAPIContext<State, Actions>() {
  const context = React.createContext<API<State, Actions> | null>(null)
  return createProviderAndHooks(context)
}

function createProviderAndHooks<State, Actions>(
  context: React.Context<API<State, Actions> | null>
) {
  function useAPI(): API<State, Actions> {
    const api = React.useContext(context)
    if (!api) {
      throw new Error('API context value has not been set')
    }
    return api
  }

  function useActions(): Actions {
    return useAPI()
  }

  function useState<T>(map: (state: State) => T): T {
    const api = useAPI()
    const [value, setValue] = React.useState(() => map(api.getState()))
    React.useEffect(() => {
      setValue(map(api.getState()))
      return api.addListener((state) => { setValue(map(state)) })
    }, [api])
    return value
  }

  interface ProviderProps {
    value: API<State, Actions>
    children: React.ReactNode
  }

  function Provider(props: ProviderProps) {
    return React.createElement(
      context.Provider, { value: props.value }, props.children)
  }

  return { Provider, useActions, useState }
}
