// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import getVPNServiceHandler, { ConnectionState, PurchasedState, Region, ServiceObserverReceiver } from '../api/braveVpn'
import * as Actions from '../actions/brave_vpn_actions'
import { ApplicationState } from '../reducers'
import AsyncActionHandler from '../../common/AsyncActionHandler'
import getNTPBrowserAPI from '../api/background'
import store from '../store'

const observer = {
  onConnectionStateChanged: (state: ConnectionState) => {
    store.dispatch(Actions.connectionStateChanged(state))
  },

  onSelectedRegionChanged: (region: Region) => {
    store.dispatch(Actions.selectedRegionChanged(region))
  },

  onPurchasedStateChanged: (state: PurchasedState, description: string) => {
    store.dispatch(Actions.purchasedStateChanged(state))
  }
}

const handler = new AsyncActionHandler()

// Initialize with current purchased state.
handler.on<PurchasedState>(Actions.initialize.getType(),
  async (store, payload) => {
    const serviceObserver = new ServiceObserverReceiver(observer)
    getVPNServiceHandler().addObserver(
      serviceObserver.$.bindNewPipeAndPassRemote())
    getNTPBrowserAPI().pageHandler.refreshVPNState()
    store.dispatch(Actions.purchasedStateChanged(payload))
  }
)

handler.on(Actions.launchVPNPanel.getType(), async (store) => {
  getNTPBrowserAPI().pageHandler.launchVPNPanel()
  getNTPBrowserAPI().pageHandler.reportVPNWidgetUsage()
})

handler.on<string>(
  Actions.openVPNAccountPage.getType(),
  async (store, payload) => {
    getNTPBrowserAPI().pageHandler.openVPNAccountPage(payload)
    getNTPBrowserAPI().pageHandler.reportVPNWidgetUsage()
  }
)

handler.on(Actions.toggleConnection.getType(), async (store) => {
  const state = store.getState() as ApplicationState
  const connectionState = state.braveVPN.connectionState
  if (
    connectionState === ConnectionState.CONNECTED ||
    connectionState === ConnectionState.CONNECTING
  ) {
    getVPNServiceHandler().disconnect()
  } else {
    getVPNServiceHandler().connect()
  }
  getNTPBrowserAPI().pageHandler.reportVPNWidgetUsage()
})

handler.on<PurchasedState>(
  Actions.purchasedStateChanged.getType(),
  async (store, purchasedState) => {
    if (purchasedState !== PurchasedState.PURCHASED) {
      return
    }

    const [{ state }, { currentRegion }] = await Promise.all([
      getVPNServiceHandler().getConnectionState(),
      getVPNServiceHandler().getSelectedRegion()
    ])

    store.dispatch(Actions.connectionStateChanged(state))
    store.dispatch(Actions.selectedRegionChanged(currentRegion))
  }
)

export default handler.middleware
