/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'

import { ConnectionState, Region } from '../api/panel_browser_api'
import * as Actions from './actions'

type RootState = {
  hasError: boolean
  isSelectingRegion: boolean
  connectionStatus: ConnectionState
  regions?: Array<Region>
  currentRegion?: Region
}

const defaultState: RootState = {
  hasError: false,
  isSelectingRegion: false,
  connectionStatus: ConnectionState.DISCONNECTED,
  regions: undefined,
  currentRegion: undefined
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

// TODO(nullhook): The handler doesnt throw an error unless if explicitly
// defined. Update the type internally so it can be infered.
reducer.on(Actions.initialized, (state, payload): RootState => {
  return {
    ...state,
    currentRegion: payload.currentRegion,
    regions: payload.regions,
    connectionStatus: payload.connectionStatus
  }
})

export default reducer
