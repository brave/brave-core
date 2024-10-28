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

export interface VPNModelState {
  vpnFeatureEnabled: boolean
  showVpnWidget: boolean
  purchased: boolean
  connectionState: ConnectionState
  connectionRegion: ConnectionRegion | null
}

export function defaultState(): VPNModelState {
  return {
    vpnFeatureEnabled: false,
    showVpnWidget: false,
    purchased: false,
    connectionState: 'disconnected',
    connectionRegion: null
  }
}

export interface VPNModel {
  getState: () => VPNModelState
  addListener: (listener: (state: VPNModelState) => void) => () => void
  setShowVpnWidget: (showVpnWidget: boolean) => void
  startTrial: () => void
  restorePurchase: () => void
  toggleConnection: () => void
  openVpnPanel: () => void
}

export function defaultModel(): VPNModel {
  const state = defaultState()
  return {
    getState() { return state },
    addListener() { return () => {} },
    setShowVpnWidget(showVpnWidget) {},
    startTrial() {},
    restorePurchase() {},
    toggleConnection() {},
    openVpnPanel() {}
  }
}
