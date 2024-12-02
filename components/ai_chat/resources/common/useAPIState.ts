// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import API from './api'

export default function useAPIState<T, Y>(api: API<Y>, defaultState: T) {
  // Intialize with global state that may have been set between module-load
  // time and the first React render.
  const [context, setContext] = React.useState<T>({
    ...defaultState,
    ...api.state
  })
  const updateFromAPIState = (state: Y) => {
    setContext((value) => ({
      ...value,
      ...state
    }))
  }

  React.useEffect(() => {
    // Update with any global state change that may have occurred between
    // first React render and first useEffect run.
    updateFromAPIState(api.state)

    // Listen for global state changes that occur after now
    const onGlobalStateChange = () => {
      updateFromAPIState(api.state)
    }
    api.addStateChangeListener(onGlobalStateChange)

    return () => {
      api.removeStateChangeListener(onGlobalStateChange)
    }
  }, [])
  return context
}
