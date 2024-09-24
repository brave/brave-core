// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createAction } from 'redux-act'
import * as BraveVPN from '../api/braveVpn'

export const toggleConnection = createAction('toggleConnection')
export const launchVPNPanel = createAction('launchVPNPanel')
export const purchasedStateChanged = createAction<BraveVPN.PurchasedState>('purchasedStateChanged')
export const connectionStateChanged = createAction<BraveVPN.ConnectionState>('connectionStateChanged')
export const selectedRegionChanged = createAction<BraveVPN.Region>('connectionStateChanged')
