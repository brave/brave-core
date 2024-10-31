/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AppState } from './app_state'
import { AppModel, defaultModel } from './app_model'

export const AppModelContext = React.createContext<AppModel>(defaultModel())

// A helper hook for importing application state into component state.
export function useAppState<T>(map: (state: AppState) => T): T {
  const model = React.useContext(AppModelContext)
  const [state, setState] = React.useState(() => map(model.getState()))
  React.useEffect(() => {
    return model.addListener((state) => {
      const result = map(state)
      if (result === state) {
        // If `map` is an identity function, then call `setState` with a new
        // object in order to ensure a re-render.
        setState({ ...result })
      } else {
        setState(result)
      }
    })
  }, [model])
  return state
}
