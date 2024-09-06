// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../../common/AsyncActionHandler'
import getPanelBrowserAPI, {
  ConnectionState,
  PurchasedState
} from '../api/panel_browser_api'
import * as Actions from './actions'
import { RootState } from './store'

const handler = new AsyncActionHandler()
type Store = MiddlewareAPI<Dispatch<AnyAction>, any>
const getState = (store: Store) =>
  store.getState() as RootState /* Helper to infer specific store type */

handler.on(Actions.connect.getType(), async () => {
  getPanelBrowserAPI().serviceHandler.connect()
})

handler.on(Actions.disconnect.getType(), async () => {
  getPanelBrowserAPI().serviceHandler.disconnect()
})

handler.on(Actions.resetConnectionState.getType(), async () => {
  getPanelBrowserAPI().serviceHandler.resetConnectionState()
})

handler.on(Actions.connectToNewRegion.getType(), async (store) => {
  const state = getState(store)

  if (!state.currentRegion) {
    console.error('Current region is not defined')
    return
  }

  getPanelBrowserAPI().serviceHandler.setSelectedRegion(state.currentRegion)
  getPanelBrowserAPI().serviceHandler.connect()
})

handler.on(Actions.connectToNewRegionAutomatically.getType(), async (store) => {
  getPanelBrowserAPI().serviceHandler.clearSelectedRegion()
  getPanelBrowserAPI().serviceHandler.connect()
})

handler.on(Actions.connectionStateChanged.getType(), async (store) => {
  const state = getState(store)

  if (state.connectionStatus === ConnectionState.CONNECT_FAILED) {
    store.dispatch(Actions.connectionFailed())
    console.warn('Connection has failed')
  }
})

handler.on(Actions.purchaseConfirmed.getType(), async (store) => {
  const [{ state }, { currentRegion }, { regions }] = await Promise.all([
    getPanelBrowserAPI().serviceHandler.getConnectionState(),
    getPanelBrowserAPI().serviceHandler.getSelectedRegion(),
    getPanelBrowserAPI().serviceHandler.getAllRegions()
  ])

  store.dispatch(
    Actions.showMainView({
      currentRegion,
      regions,
      connectionStatus:
        state === ConnectionState.CONNECT_FAILED
          ? ConnectionState.DISCONNECTED
          : state /* Treat connection failure on startup as disconnected */,
      expired: false
    })
  )
})

handler.on(Actions.purchaseExpired.getType(), async (store) => {
  const [{ currentRegion }, { regions }] = await Promise.all([
    getPanelBrowserAPI().serviceHandler.getSelectedRegion(),
    getPanelBrowserAPI().serviceHandler.getAllRegions()
  ])

  store.dispatch(
    Actions.showMainView({
      currentRegion,
      regions,
      connectionStatus: ConnectionState.DISCONNECTED,
      expired: true
    })
  )
})

handler.on(Actions.initialize.getType(), async (store) => {
  const [{ state }, { urls }] = await Promise.all([
    getPanelBrowserAPI().serviceHandler.getPurchasedState(),
    getPanelBrowserAPI().serviceHandler.getProductUrls()
  ])

  store.dispatch(
    Actions.initialized({
      productUrls: urls
    })
  )

  if (state.state === PurchasedState.NOT_PURCHASED) {
    store.dispatch(Actions.showSellView())
  }

  if (state.state === PurchasedState.PURCHASED) {
    store.dispatch(Actions.purchaseConfirmed())
  }

  if (state.state === PurchasedState.SESSION_EXPIRED) {
    store.dispatch(Actions.purchaseExpired())
  }

  if (state.state === PurchasedState.LOADING) {
    store.dispatch(Actions.showLoadingView())
  }
  if (state.state === PurchasedState.FAILED) {
    store.dispatch(
      Actions.purchaseFailed({
        state: PurchasedState.FAILED,
        stateDescription: state.description
      })
    )
  }
})

export default handler.middleware
