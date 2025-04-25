/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { VpnAPI } from '../../api/vpn'
import { createContextProvider } from './model_context'

const { Provider, useActions, useState } = createContextProvider(
  React.createContext<VpnAPI | null>(null))

export {
  Provider as VpnProvider,
  useActions as useVpnActions,
  useState as useVpnState
}
