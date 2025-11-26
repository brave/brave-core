/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  defaultBackgroundState,
  getCurrentBackground,
} from '../state/background_state'
import { createBackgroundHandler } from '../state/background_handler'
import { createStateProvider } from '../lib/state_provider'

export const BackgroundProvider = createStateProvider(
  defaultBackgroundState(),
  createBackgroundHandler,
)

export const useBackgroundState = BackgroundProvider.useState
export const useBackgroundActions = BackgroundProvider.useActions

export function useCurrentBackground() {
  const state = useBackgroundState((s) => ({ ...s }))
  return React.useMemo(() => getCurrentBackground(state), Object.values(state))
}
