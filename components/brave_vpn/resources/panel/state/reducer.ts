/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'

import { ConnectionState, Region, ProductUrls } from '../api/panel_browser_api'
import * as Actions from './actions'
import { ViewType } from './component_types'

type RootState = {
  hasError: boolean
  isSelectingRegion: boolean
  expired: boolean
  outOfCredentials: boolean
  connectionStatus: ConnectionState
  regions: Region[]
  currentRegion: Region
  productUrls?: ProductUrls
  currentView: ViewType,
  stateDescription?: string
}

const defaultState: RootState = {
  hasError: false,
  isSelectingRegion: false,
  expired: false,
  outOfCredentials: false,
  connectionStatus: ConnectionState.DISCONNECTED,
  regions: [],
  currentRegion: new Region(),
  currentView: ViewType.Loading
}

const reducer = createReducer<RootState>({}, defaultState)

reducer.on(Actions.connect, (state): RootState => {
  return {
    ...state,
    hasError: false,
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
    hasError: false,
    currentRegion: payload.region
  }
})

reducer.on(Actions.connectToNewRegionAutomatically,
           (state, payload): RootState => {
  return {
    ...state,
    isSelectingRegion: false,
    hasError: false
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
    hasError: payload.connectionStatus === ConnectionState.CONNECTED ? false : state.hasError,
    connectionStatus: payload.connectionStatus
  }
})

reducer.on(Actions.selectedRegionChanged, (state, payload): RootState => {
  return {
    ...state,
    currentRegion: payload.region
  }
})

reducer.on(Actions.purchaseConfirmed, (state): RootState => {
  return {
    ...state,
    currentView: ViewType.Main
  }
})

reducer.on(Actions.purchaseFailed, (state, payload): RootState => {
  return {
    ...state,
    currentView: ViewType.PurchaseFailed,
    stateDescription: payload.stateDescription
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

reducer.on(Actions.outOfCredentials, (state, payload): RootState => {
  return {
      ...state,
      expired: false,
      stateDescription: payload.description || '',
      outOfCredentials: true,
      currentView: ViewType.Main
    }
})

reducer.on(Actions.initialized, (state, payload): RootState => {
  return {
    ...state,
    productUrls: payload.productUrls
  }
})

reducer.on(Actions.resetConnectionState, (state): RootState => {
  return {
    ...state,
    hasError: false,
    connectionStatus: ConnectionState.DISCONNECTED,
    currentView: ViewType.Main
  }
})

// TODO(nullhook): The handler doesnt throw an error unless if explicitly
// defined. Update the type internally so it can be infered.
reducer.on(Actions.showMainView, (state, payload): RootState => {
  return {
    ...state,
    expired: payload.expired,
    outOfCredentials: payload.outOfCredentials,
    currentRegion: payload.currentRegion,
    regions: payload.regions,
    connectionStatus: payload.connectionStatus,
    stateDescription: payload.stateDescription,
    currentView: ViewType.Main
  }
})

export default reducer
