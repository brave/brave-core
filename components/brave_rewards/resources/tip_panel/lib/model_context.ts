/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Model, ModelState, defaultModel } from './model'

export const ModelContext = React.createContext<Model>(defaultModel())

// A helper hook for importing model state into component state.
export function useModelState<T> (map: (state: ModelState) => T): T {
  const model = React.useContext(ModelContext)
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
