// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveVPNState } from '../../../reducers/brave_vpn'
import * as BraveVPN from '../../../api/braveVpn'

export default function getBraveVPNState (): BraveVPNState {
  return {
    purchasedState: BraveVPN.PurchasedState.NOT_PURCHASED,
    connectionState: BraveVPN.ConnectionState.DISCONNECTED,
    selectedRegion: new BraveVPN.Region()
  }
}