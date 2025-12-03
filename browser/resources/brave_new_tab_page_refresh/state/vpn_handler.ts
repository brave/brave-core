/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import * as mojom from 'gen/brave/components/brave_vpn/common/mojom/brave_vpn.mojom.m'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { Store } from '../lib/store'
import { debounce } from '$web-common/debounce'
import {
  VpnState,
  VpnActions,
  defaultVpnActions,
  ConnectionState,
} from './vpn_state'

export function createVpnHandler(store: Store<VpnState>): VpnActions {
  if (!loadTimeData.getBoolean('vpnFeatureEnabled')) {
    store.update({ initialized: true })
    return defaultVpnActions()
  }

  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  const vpnService = mojom.ServiceHandler.getRemote()

  store.update({ vpnFeatureEnabled: true })

  async function updateConnectionInfo() {
    const { state: purchasedState } = await vpnService.getPurchasedState()
    const vpnPurchased = purchasedState.state === mojom.PurchasedState.PURCHASED

    store.update({ vpnPurchased })

    if (!vpnPurchased) {
      store.update({
        vpnConnectionState: ConnectionState.DISCONNECTED,
        vpnConnectionRegion: null,
      })
      return
    }

    const [{ state: connectionState }, { currentRegion }] = await Promise.all([
      vpnService.getConnectionState(),
      vpnService.getSelectedRegion(),
    ])

    store.update({
      vpnConnectionState: connectionState,
      vpnConnectionRegion: currentRegion ?? null,
    })
  }

  async function updatePrefs() {
    const { showVpnWidget } = await handler.getShowVPNWidget()
    store.update({ vpnFeatureEnabled: true, showVpnWidget })
  }

  async function loadData() {
    await newTabProxy.handler.reloadVPNPurchasedState()
    await Promise.all([updatePrefs(), updateConnectionInfo()])
    store.update({ initialized: true })
  }

  newTabProxy.addListeners({
    onVPNStateUpdated: debounce(updatePrefs, 10),
  })

  const vpnServiceObserver = new mojom.ServiceObserverReceiver({
    onConnectionStateChanged: updateConnectionInfo,
    onSelectedRegionChanged: updateConnectionInfo,
    onPurchasedStateChanged: updateConnectionInfo,
    onSmartProxyRoutingStateChanged: (enabled: boolean) => {},
  })

  vpnService.addObserver(vpnServiceObserver.$.bindNewPipeAndPassRemote())

  loadData()

  return {
    setShowVpnWidget(showVpnWidget) {
      handler.setShowVPNWidget(showVpnWidget)
    },

    startVpnTrial() {
      handler.reportVPNWidgetUsage()
      handler.openVPNAccountPage(mojom.ManageURLType.CHECKOUT)
    },

    restoreVpnPurchase() {
      handler.reportVPNWidgetUsage()
      handler.openVPNAccountPage(mojom.ManageURLType.RECOVER)
    },

    toggleVpnConnection() {
      const { vpnConnectionState } = store.getState()
      switch (vpnConnectionState) {
        case ConnectionState.CONNECTED:
        case ConnectionState.CONNECTING:
          handler.reportVPNWidgetUsage()
          vpnService.disconnect()
          break
        case ConnectionState.DISCONNECTED:
        case ConnectionState.DISCONNECTING:
          handler.reportVPNWidgetUsage()
          vpnService.connect()
          break
        default:
          console.error('Unhandled ConnectionState', vpnConnectionState)
          break
      }
    },

    openVpnPanel() {
      handler.reportVPNWidgetUsage()
      handler.openVPNPanel()
    },
  }
}
