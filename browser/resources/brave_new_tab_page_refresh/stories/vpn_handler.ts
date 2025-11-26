/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Store } from '../lib/store'

import {
  VpnState,
  VpnActions,
  defaultVpnActions,
  ConnectionState,
} from '../state/vpn_state'

export function createVpnHandler(store: Store<VpnState>): VpnActions {
  store.update({
    initialized: true,
    vpnFeatureEnabled: true,
    showVpnWidget: true,
    vpnPurchased: true,
    vpnConnectionState: ConnectionState.CONNECTED,
    vpnConnectionRegion: {
      name: '',
      continent: '',
      countryIsoCode: '',
      regionPrecision: '',
      cities: [],
      latitude: 0,
      longitude: 0,
      serverCount: 0,
      isAutomatic: false,
      smartRoutingProxyState: 'none',
      country: 'Brazil',
      namePretty: 'Rio de Janeiro',
    },
  })

  return {
    ...defaultVpnActions(),

    setShowVpnWidget(showVpnWidget) {
      store.update({ showVpnWidget })
    },

    toggleVpnConnection() {
      store.update((state) => {
        return {
          vpnConnectionState:
            state.vpnConnectionState === ConnectionState.CONNECTED
            || state.vpnConnectionState === ConnectionState.CONNECTING
              ? ConnectionState.DISCONNECTED
              : ConnectionState.CONNECTED,
        }
      })
    },
  }
}
