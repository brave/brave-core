/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  ConnectionState,
  Region,
} from 'gen/brave/components/brave_vpn/common/mojom/brave_vpn.mojom.m'

export { ConnectionState, Region }

export interface VpnState {
  initialized: boolean
  vpnFeatureEnabled: boolean
  showVpnWidget: boolean
  vpnPurchased: boolean
  vpnConnectionState: ConnectionState
  vpnConnectionRegion: Region | null
}

export function defaultVpnState(): VpnState {
  return {
    initialized: false,
    vpnFeatureEnabled: false,
    showVpnWidget: false,
    vpnPurchased: false,
    vpnConnectionState: ConnectionState.DISCONNECTED,
    vpnConnectionRegion: null,
  }
}

export interface VpnActions {
  setShowVpnWidget: (showVpnWidget: boolean) => void
  startVpnTrial: () => void
  restoreVpnPurchase: () => void
  toggleVpnConnection: () => void
  openVpnPanel: () => void
}

export function defaultVpnActions(): VpnActions {
  return {
    setShowVpnWidget(showVpnWidget) {},
    startVpnTrial() {},
    restoreVpnPurchase() {},
    toggleVpnConnection() {},
    openVpnPanel() {},
  }
}
