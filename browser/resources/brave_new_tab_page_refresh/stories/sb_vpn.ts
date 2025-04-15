/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Store } from '../lib/store'

import {
  VPNState,
  VPNActions,
  defaultVPNState,
  defaultVPNActions } from '../models/vpn'

export function initializeVPN(store: Store<VPNState>): VPNActions {
  store.update({
    ...defaultVPNState(),
    vpnFeatureEnabled: true,
    showVpnWidget: true,
    vpnPurchased: true,
    vpnConnectionState: 'connecting',
    vpnConnectionRegion: {
      country: 'Brazil',
      name: 'Rio de Janeiro'
    }
  })

  return {
    ...defaultVPNActions(),

    setShowVpnWidget(showVpnWidget) {
      store.update({ showVpnWidget })
    },

    toggleVpnConnection() {
      store.update((state) => {
        return {
          vpnConnectionState:
            state.vpnConnectionState === 'connected' ||
            state.vpnConnectionState === 'connecting'
                ? 'disconnected'
                : 'connected'
        }
      })
    }
  }
}
