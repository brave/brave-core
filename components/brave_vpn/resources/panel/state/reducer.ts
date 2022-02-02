/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'

import { ConnectionState, Region, ProductUrls } from '../api/panel_browser_api'
import * as Actions from './actions'
import { ViewType } from './component_types'

type RootState = {
  hasError: boolean
  isSelectingRegion: boolean
  connectionStatus: ConnectionState
  regions?: Region[]
  currentRegion?: Region
  productUrls?: ProductUrls
  currentView: ViewType | null
}

const defaultState: RootState = {
  hasError: false,
  isSelectingRegion: false,
  connectionStatus: ConnectionState.DISCONNECTED,
  regions: undefined,
  currentRegion: undefined,
  currentView: null
}

const reducer = createReducer<RootState>({}, defaultState)

reducer.on(Actions.connect, (state): RootState => {
  return {
    ...state,
    connectionStatus: ConnectionState.CONNECTING
  }
})

reducer.on(Actions.disconnect, (state): RootState => {
  // Some OS VPN API may fire disconnecting state. We can safely assume
  // this on all OS to keep the UI consistent
  return {
    ...state,
    connectionStatus: ConnectionState.DISCONNECTING
  }
})

reducer.on(Actions.connectionFailed, (state): RootState => {
  return {
    ...state,
    hasError: true
  }
})

reducer.on(Actions.connectToNewRegion, (state, payload): RootState => {
  return {
    ...state,
    isSelectingRegion: false,
    currentRegion: payload.region
  }
})

reducer.on(Actions.toggleRegionSelector, (state, payload): RootState => {
  return {
    ...state,
    isSelectingRegion: payload.isSelectingRegion
  }
})

reducer.on(Actions.connectionStateChanged, (state, payload): RootState => {
  return {
    ...state,
    connectionStatus: payload.connectionStatus
  }
})

reducer.on(Actions.retryConnect, (state): RootState => {
  return {
    ...state,
    hasError: false
  }
})

reducer.on(Actions.purchaseConfirmed, (state): RootState => {
  return {
    ...state,
    currentView: null
  }
})

reducer.on(Actions.showSellView, (state): RootState => {
  return {
    ...state,
    currentView: ViewType.Sell
  }
})

reducer.on(Actions.showLoadingView, (state): RootState => {
  return {
    ...state,
    currentView: ViewType.Loading
  }
})

reducer.on(Actions.initialized, (state, payload): RootState => {
  return {
    ...state,
    productUrls: payload.productUrls
  }
})

// TODO(nullhook): The handler doesnt throw an error unless if explicitly
// defined. Update the type internally so it can be infered.
reducer.on(Actions.showMainView, (state, payload): RootState => {
  return {
    ...state,
    currentRegion: payload.currentRegion,
    regions: payload.regions,
    connectionStatus: payload.connectionStatus,
    currentView: ViewType.Main
  }
})

export default reducer
