/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AppModel, AppActions, AppState, defaultModel } from '../../models/app_model'

const Context = React.createContext<AppModel>(defaultModel())

interface Props {
  model: AppModel
  children: React.ReactNode
}

export function AppModelContext(props: Props) {
  return (
    <Context.Provider value={props.model}>
      {props.children}
    </Context.Provider>
  )
}

export function useAppActions() : AppActions {
  return React.useContext(Context)
}

export function useAppState<T>(map: (state: AppState) => T): T {
  const model = React.useContext(Context)
  const [value, setValue] = React.useState(() => map(model.getState()))
  React.useEffect(() => {
    return model.addListener((state) => {
      const result = map(state)
      if (Object.is(result, state)) {
        // If `map` is the identity function, then call `setState` with a new
        // object in order to ensure a re-render.
        setValue({ ...result })
      } else {
        setValue(result)
      }
    })
  }, [model])
  return value
}
