// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createReducer } from 'redux-act'
import * as Actions from '../../actions/brave_vpn_actions'
import * as BraveVPN from '../../api/braveVpn'

export type BraveVPNState = {
  purchasedState: BraveVPN.PurchasedState
  connectionState: BraveVPN.ConnectionState
  selectedRegion: BraveVPN.Region
}

const defaultState: BraveVPNState = {
  purchasedState: BraveVPN.PurchasedState.NOT_PURCHASED,
  connectionState: BraveVPN.ConnectionState.DISCONNECTED,
  selectedRegion: new BraveVPN.Region()
}

const reducer = createReducer<BraveVPNState>({}, defaultState)

reducer.on(Actions.connectionStateChanged, (state, payload): BraveVPNState => {
  console.log('connection state', payload)
  return {
    ...state,
    connectionState: payload
  }
})

reducer.on(Actions.purchasedStateChanged, (state, payload): BraveVPNState => {
  console.log('purchased state', payload)
  return {
    ...state,
    purchasedState: payload
  }
})

reducer.on(Actions.selectedRegionChanged, (state, payload): BraveVPNState => {
  console.log('selected region', payload)
  return {
    ...state,
    selectedRegion: payload
  }
})

export default reducer
