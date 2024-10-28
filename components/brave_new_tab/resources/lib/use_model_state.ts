/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

type StateListener<State> = (state: State) => void

interface Model<State> {
  getState: () => State
  addListener: (listener: StateListener<State>) => () => void
}

// A helper for importing model state into component state. This helper can be
// used by model context modules when implementing "useFooState" hooks.
export function useModelState<State, T>(
  model: Model<State>,
  map: (state: State) => T
): T {
  const [value, setValue] = React.useState(() => map(model.getState()))
  React.useEffect(() => {
    return model.addListener((state) => {
      const result = map(state)
      if (result === value) {
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
