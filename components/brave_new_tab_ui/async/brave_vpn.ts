// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import getVPNServiceHandler, { ServiceObserverReceiver, ConnectionState, PurchasedState, Region } from '../api/braveVpn'
import * as Actions from '../actions/brave_vpn_actions'
import store from '../store'
import { ApplicationState } from '../reducers'
import AsyncActionHandler from '../../common/AsyncActionHandler'
import getNTPBrowserAPI from '../api/background'

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

handler.on(Actions.initialize.getType(),
  async (store) => {
    const serviceObserver = new ServiceObserverReceiver(observer)
    getVPNServiceHandler().addObserver(
      serviceObserver.$.bindNewPipeAndPassRemote())
    getNTPBrowserAPI().pageHandler.refreshVPNState()

    // If already purchased state, refresh could be completed before
    // start observing. Fetch current state.
    const { state } = await getVPNServiceHandler().getPurchasedState()
    store.dispatch(Actions.purchasedStateChanged(state.state))
  }
)

handler.on(Actions.launchVPNPanel.getType(),
  async (store) => {
    getNTPBrowserAPI().pageHandler.launchVPNPanel()
  }
)

handler.on(Actions.openVPNAccountPage.getType(),
  async (store) => {
    getNTPBrowserAPI().pageHandler.openVPNAccountPage()
  }
)

handler.on(Actions.toggleConnection.getType(),
  async (store) => {
    const state = store.getState() as ApplicationState
    const connectionState = state.braveVPN.connectionState
    if (connectionState === ConnectionState.CONNECTED ||
        connectionState === ConnectionState.CONNECTING) {
      getVPNServiceHandler().disconnect();
    } else {
      getVPNServiceHandler().connect()
    }
  }
)

handler.on<PurchasedState>(Actions.purchasedStateChanged.getType(),
  async (store, purchasedState) => {
    if (purchasedState !== PurchasedState.PURCHASED) {
      return
    }

    const [{ state }, { currentRegion }] = await Promise.all([
      getVPNServiceHandler().getConnectionState(),
      getVPNServiceHandler().getSelectedRegion(),
    ])

    store.dispatch(Actions.connectionStateChanged(state))
    store.dispatch(Actions.selectedRegionChanged(currentRegion))
  }
)

export default handler.middleware
