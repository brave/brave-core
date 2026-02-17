/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  defaultBackgroundStore,
  getCurrentBackground,
} from '../state/background_store'
import { createUseStateHook } from '$web-common/state_store_hooks'

export const BackgroundContext = React.createContext(defaultBackgroundStore())

export const useBackgroundState = createUseStateHook(BackgroundContext)

export function useBackgroundActions() {
  return useBackgroundState((s) => s.actions)
}

export function useCurrentBackground() {
  const state = useBackgroundState((s) => ({ ...s }))
  return React.useMemo(() => getCurrentBackground(state), Object.values(state))
}
