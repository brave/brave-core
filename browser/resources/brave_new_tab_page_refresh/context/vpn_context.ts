/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { defaultVpnState } from '../state/vpn_state'
import { createVpnHandler } from '../state/vpn_handler'
import { createStateProvider } from '../lib/state_provider'

export const VpnProvider = createStateProvider(
  defaultVpnState(),
  createVpnHandler,
)

export const useVpnState = VpnProvider.useState
export const useVpnActions = VpnProvider.useActions
