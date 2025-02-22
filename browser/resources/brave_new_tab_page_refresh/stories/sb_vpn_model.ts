/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore } from '../lib/store'

import {
  VPNModel,
  VPNModelState,
  defaultModel,
  defaultState } from '../models/vpn_model'

export function createVPNModel(): VPNModel {
  const store = createStore<VPNModelState>({
    ...defaultState(),

    vpnFeatureEnabled: true,
    showVpnWidget: true,
    purchased: false,
    connectionState: 'connecting',
    connectionRegion: {
      country: 'Brazil',
      name: 'Rio de Janeiro'
    }
  })

  return {
    ...defaultModel(),

    getState: store.getState,
    addListener: store.addListener,

    setShowVpnWidget(showVpnWidget) {
      store.update({ showVpnWidget })
    },

    toggleConnection() {
      store.update((state) => {
        return {
          connectionState:
            state.connectionState === 'connected' ||
            state.connectionState === 'connecting'
                ? 'disconnected'
                : 'connected'
        }
      })
    }
  }
}
