/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StateStore } from '$web-common/state_store'

export function createUseStateHook<State>(
  context: React.Context<StateStore<State>>,
) {
  function useStateStore<T>(map: (state: State) => T): T {
    const store = React.useContext(context)
    const [value, setValue] = React.useState(() => map(store.getState()))
    React.useEffect(() => {
      setValue(map(store.getState()))
      return store.addListener((state) => {
        setValue(map(state))
      })
    }, [store])
    return value
  }
  return useStateStore
}
