/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { defaultNewTabStore } from '../state/new_tab_store'
import { createUseStateHook } from '$web-common/state_store_hooks'

export const NewTabContext = React.createContext(defaultNewTabStore())

export const useNewTabState = createUseStateHook(NewTabContext)

export function useNewTabActions() {
  return useNewTabState((s) => s.actions)
}
