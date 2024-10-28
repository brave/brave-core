/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import * as mojom from 'gen/brave/components/brave_vpn/common/mojom/brave_vpn.mojom.m'

import { NewTabPageProxy } from './new_tab_page_proxy'
import { createStore } from '../lib/store'
import { debounceListener } from './debounce_listener'

import {
  VPNModel,
  ConnectionState,
  defaultState,
  defaultModel } from '../models/vpn_model'

function mapConnectionState(state: mojom.ConnectionState): ConnectionState {
  switch (state) {
    case mojom.ConnectionState.CONNECTED:
      return 'connected'
    case mojom.ConnectionState.CONNECTING:
      return 'connecting'
    case mojom.ConnectionState.DISCONNECTING:
      return 'disconnecting'
    default:
      return 'disconnected'
  }
}

export function createVPNModel(): VPNModel {
  if (!loadTimeData.getBoolean('vpnFeatureEnabled')) {
    return defaultModel()
  }

  const store = createStore({ ...defaultState(), vpnFeatureEnabled: true })
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  const vpnService = mojom.ServiceHandler.getRemote()

  async function updateConnectionInfo() {
    const { state: purchasedState } = await vpnService.getPurchasedState()
    const purchased = purchasedState.state === mojom.PurchasedState.PURCHASED

    store.update({ purchased })

    if (!purchased) {
      store.update({
        connectionState: 'disconnected',
        connectionRegion: null
      })
      return
    }

    const [
      { state: connectionState },
      { currentRegion }
    ] = await Promise.all([
      vpnService.getConnectionState(),
      vpnService.getSelectedRegion()
    ])

    store.update({
      connectionState: mapConnectionState(connectionState),
      connectionRegion: !currentRegion ? null : {
        name: currentRegion.namePretty,
        country: currentRegion.country
      }
    })
  }

  async function updatePrefs() {
    const { showVpnWidget } = await handler.getShowVPNWidget()
    store.update({ vpnFeatureEnabled: true, showVpnWidget })
  }

  async function loadData() {
    await newTabProxy.handler.reloadVPNPurchasedState()
    await Promise.all([
      updatePrefs(),
      updateConnectionInfo()
    ])
  }

  newTabProxy.addListeners({
    onVPNStateUpdated: debounceListener(updatePrefs)
  })

  const vpnServiceObserver = new mojom.ServiceObserverReceiver({
    onConnectionStateChanged(state) {
      updateConnectionInfo()
    },

    onSelectedRegionChanged(region) {
      updateConnectionInfo()
    },

    onPurchasedStateChanged(state, description) {
      updateConnectionInfo()
    }
  })

  vpnService.addObserver(vpnServiceObserver.$.bindNewPipeAndPassRemote())

  loadData()

  return {
    getState: store.getState,

    addListener: store.addListener,

    async setShowVpnWidget(showVpnWidget) {
      await handler.setShowVPNWidget(showVpnWidget)
    },

    startTrial() {
      handler.reportVPNWidgetUsage()
      handler.openVPNAccountPage(mojom.ManageURLType.CHECKOUT)
    },

    restorePurchase() {
      handler.reportVPNWidgetUsage()
      handler.openVPNAccountPage(mojom.ManageURLType.RECOVER)
    },

    toggleConnection() {
      const { connectionState } = store.getState()
      switch (connectionState) {
        case 'connected':
        case 'connecting':
          handler.reportVPNWidgetUsage()
          vpnService.disconnect()
          break
        case 'disconnected':
        case 'disconnecting':
          handler.reportVPNWidgetUsage()
          vpnService.connect()
          break
      }
    },

    openVpnPanel() {
      handler.reportVPNWidgetUsage()
      handler.openVPNPanel()
    }
  }
}
