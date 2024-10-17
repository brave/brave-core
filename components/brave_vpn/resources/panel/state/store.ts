/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore, applyMiddleware } from 'redux'

import reducer from './reducer'
import asyncHandler from './async'
import * as Actions from './actions'
import getPanelBrowserAPI, { ServiceObserverReceiver, ConnectionState, PurchasedState, Region } from '../api/panel_browser_api'

const store = createStore(
  reducer,
  applyMiddleware(asyncHandler)
)

// Register the observer earlier
const observer = {
  onConnectionStateChanged: (connectionStatus: ConnectionState) => {
    store.dispatch(Actions.connectionStateChanged({ connectionStatus }))
  },
  onSelectedRegionChanged: (region: Region) => {
    store.dispatch(Actions.selectedRegionChanged({ region }))
  },

  onPurchasedStateChanged: (state: PurchasedState, description: string) => {
    switch (state) {
      case PurchasedState.PURCHASED:
        store.dispatch(Actions.purchaseConfirmed())
        break
      case PurchasedState.SESSION_EXPIRED:
        store.dispatch(Actions.purchaseExpired())
        break
      case PurchasedState.OUT_OF_CREDENTIALS:
        store.dispatch(Actions.outOfCredentials({description}))
        break
      case PurchasedState.FAILED:
        store.dispatch(Actions.purchaseFailed({
          state: PurchasedState.FAILED, stateDescription: description
        }))
        break
      case PurchasedState.NOT_PURCHASED:
        store.dispatch(Actions.showSellView())
        break
      case PurchasedState.LOADING:
        store.dispatch(Actions.showLoadingView())
        break
    }
  }
}
const serviceObserver = new ServiceObserverReceiver(observer)
getPanelBrowserAPI().serviceHandler.addObserver(
  serviceObserver.$.bindNewPipeAndPassRemote())

store.dispatch(Actions.initialize())

// Infer state type from the store itself
export type RootState = ReturnType<typeof store.getState>
export type AppDispatch = typeof store.dispatch

export default store
