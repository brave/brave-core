/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { defaultAppState } from '../state/app_state'
import { createAppHandler } from '../state/app_handler'
import { createStateProvider } from '../lib/state_provider'

export const AppProvider = createStateProvider(
  defaultAppState(),
  createAppHandler,
)

export const useAppState = AppProvider.useState
export const useAppActions = AppProvider.useActions
