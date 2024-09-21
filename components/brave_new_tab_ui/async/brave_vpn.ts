// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import getWidgetBrowserAPI, { ServiceObserverReceiver, ConnectionState, PurchasedState, Region } from '../api/braveVpn'
import * as Actions from '../actions/brave_vpn_actions'
import store from '../store'
import { ApplicationState } from '../reducers'
import AsyncActionHandler from '../../common/AsyncActionHandler'

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

const serviceObserver = new ServiceObserverReceiver(observer)
getWidgetBrowserAPI().serviceHandler.addObserver(
  serviceObserver.$.bindNewPipeAndPassRemote())

getWidgetBrowserAPI().serviceHandler.initialize();

const handler = new AsyncActionHandler()

handler.on(Actions.toggleConnection.getType(),
  async (store) => {
    const state = store.getState() as ApplicationState
    const connectionState = state.braveVPN.connectionState
    if (connectionState === ConnectionState.CONNECTED ||
        connectionState === ConnectionState.CONNECTING) {
      getWidgetBrowserAPI().serviceHandler.disconnect();
    } else {
      getWidgetBrowserAPI().serviceHandler.connect()
    }
  }
)

handler.on<PurchasedState>(Actions.purchasedStateChanged.getType(),
  async (store, purchased_state) => {
    if (purchased_state !== PurchasedState.PURCHASED) {
      return
    }

    const [ { state }, { currentRegion }] = await Promise.all([
      getWidgetBrowserAPI().serviceHandler.getConnectionState(),
      getWidgetBrowserAPI().serviceHandler.getSelectedRegion(),
    ])

    store.dispatch(Actions.connectionStateChanged(state))
    store.dispatch(Actions.selectedRegionChanged(currentRegion))
  }
)

export default handler.middleware
