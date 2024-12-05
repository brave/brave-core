// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import getVPNServiceHandler, { ConnectionState, PurchasedState, ManageURLType, Region, ServiceObserverReceiver } from '../api/braveVpn'
import * as Actions from '../actions/brave_vpn_actions'
import { ApplicationState } from '../reducers'
import AsyncActionHandler from '../../common/AsyncActionHandler'
import getNTPBrowserAPI from '../api/background'
import store from '../store'

// We added @ts-ignore to avoid build failure on linux as
// we don't support vpn on that platform. VPN related methods
// in PageHandler aren't defined on linux. And we don't call
// them all in linux. So, safe to ignore here.

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
    // eslint-disable-next-line @typescript-eslint/prefer-ts-expect-error
    // @ts-ignore
    getNTPBrowserAPI().pageHandler.refreshVPNState()
    store.dispatch(Actions.purchasedStateChanged(payload))
  }
)

handler.on(Actions.launchVPNPanel.getType(), async (store) => {
  // eslint-disable-next-line @typescript-eslint/prefer-ts-expect-error
  // @ts-ignore
  getNTPBrowserAPI().pageHandler.launchVPNPanel()
  // eslint-disable-next-line @typescript-eslint/prefer-ts-expect-error
  // @ts-ignore
  getNTPBrowserAPI().pageHandler.reportVPNWidgetUsage()
})

handler.on<ManageURLType>(
  Actions.openVPNAccountPage.getType(),
  async (store, payload) => {
    // eslint-disable-next-line @typescript-eslint/prefer-ts-expect-error
    // @ts-ignore
    getNTPBrowserAPI().pageHandler.openVPNAccountPage(payload)
    // eslint-disable-next-line @typescript-eslint/prefer-ts-expect-error
    // @ts-ignore
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
  // eslint-disable-next-line @typescript-eslint/prefer-ts-expect-error
  // @ts-ignore
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
