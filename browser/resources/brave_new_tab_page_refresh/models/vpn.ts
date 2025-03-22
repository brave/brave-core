/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export type ConnectionState =
  'connecting' |
  'connected' |
  'disconnecting' |
  'disconnected'

interface ConnectionRegion {
  country: string
  name: string
}

export interface VPNState {
  vpnFeatureEnabled: boolean
  showVpnWidget: boolean
  vpnPurchased: boolean
  vpnConnectionState: ConnectionState
  vpnConnectionRegion: ConnectionRegion | null
}

export function defaultVPNState(): VPNState {
  return {
    vpnFeatureEnabled: false,
    showVpnWidget: false,
    vpnPurchased: false,
    vpnConnectionState: 'disconnected',
    vpnConnectionRegion: null
  }
}

export interface VPNActions {
  setShowVpnWidget: (showVpnWidget: boolean) => void
  startVpnTrial: () => void
  restoreVpnPurchase: () => void
  toggleVpnConnection: () => void
  openVpnPanel: () => void
}

export function defaultVPNActions(): VPNActions {
  return {
    setShowVpnWidget(showVpnWidget) {},
    startVpnTrial() {},
    restoreVpnPurchase() {},
    toggleVpnConnection() {},
    openVpnPanel() {}
  }
}
