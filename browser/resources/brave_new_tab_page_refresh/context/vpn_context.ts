/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { VpnAPI } from '../api/vpn_api'
import { createAPIProvider } from '../lib/api_provider'

export const {
  Provider: VpnProvider,
  useState: useVpnState,
  useActions: useVpnActions } =
    createAPIProvider(React.createContext<VpnAPI | null>(null))
