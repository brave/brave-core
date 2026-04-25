/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { defaultVpnStore } from '../state/vpn_store'
import { createUseStateHook } from '$web-common/state_store_hooks'

export const VpnContext = React.createContext(defaultVpnStore())

export const useVpnState = createUseStateHook(VpnContext)

export function useVpnActions() {
  return useVpnState((s) => s.actions)
}
