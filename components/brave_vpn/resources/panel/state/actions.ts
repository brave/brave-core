// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { createAction } from 'redux-act'

import { ConnectionState, Region } from '../api/panel_browser_api'

export type ToggleRegionSelectorPayload = {
  isSelectingRegion: boolean
}

export type ConnectionStatePayload = {
  connectionStatus: ConnectionState
}

export type ConnectToNewRegionPayload = {
  region: Region
}

export type InitializedPayload = {
  currentRegion: Region
  regions: Array<Region>
  connectionStatus: ConnectionState
}

export const connect = createAction('connect')
export const disconnect = createAction('disconnect')
export const initialize = createAction('initialize')
export const connectionFailed = createAction('connectionFailed')
export const retryConnect = createAction('retryConnect')

export const initialized = createAction<InitializedPayload>('initialized')
export const toggleRegionSelector = createAction<ToggleRegionSelectorPayload>('toggleRegionSelector', (isSelectingRegion) => ({ isSelectingRegion }))
export const connectionStateChanged = createAction<ConnectionStatePayload>('connectionStateChanged')
export const connectToNewRegion = createAction<ConnectToNewRegionPayload>('connectToNewRegion', (region) => ({ region }))
