/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { API } from '../../lib/api'

// Creates a context provider component and associated hooks for the specified
// API context.
export function createContextProvider<State, Actions>(
  context: React.Context<API<State, Actions> | null>
) {
  function useModel(): API<State, Actions> {
    const model = React.useContext(context)
    if (!model) {
      throw new Error('Model context has not been set')
    }
    return model
  }

  function useActions(): Actions {
    return useModel()
  }

  function useState<T>(map: (state: State) => T): T {
    const model = useModel()
    const [value, setValue] = React.useState(() => map(model.getState()))
    React.useEffect(() => {
      setValue(map(model.getState()))
      return model.addListener((state) => { setValue(map(state)) })
    }, [model])
    return value
  }

  function Provider(
    props: { value: API<State, Actions>, children: React.ReactNode }
  ) {
    return (
      <context.Provider value={props.value}>
        {props.children}
      </context.Provider>
    )
  }

  return { Provider, useActions, useState }
}
