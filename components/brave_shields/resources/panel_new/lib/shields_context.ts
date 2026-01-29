/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { createUseStateHook } from './state_store_hooks'
import { defaultShieldsStore, ShieldsActions } from './shields_store'

export const ShieldsContext = React.createContext(defaultShieldsStore())

export const useShieldsState = createUseStateHook(ShieldsContext)

export function useShieldsActions(): ShieldsActions {
  return useShieldsState((s) => s.actions)
}
