/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { defaultAppStore } from './app_store'
import { createUseStateHook } from '../../rewards_page/lib/state_store_hooks'

export const AppContext = React.createContext(defaultAppStore())

export const useAppState = createUseStateHook(AppContext)

export function useAppActions() {
  return useAppState((s) => s.actions)
}
