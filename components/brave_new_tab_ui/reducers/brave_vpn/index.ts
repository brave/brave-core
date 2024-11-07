// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createReducer } from 'redux-act'
import * as Actions from '../../actions/brave_vpn_actions'
import * as BraveVPN from '../../api/braveVpn'

// Set true to initialzied when final purchased state is received once.
// Before that, vpn card is not rendered.
export type BraveVPNState = {
  purchasedState: BraveVPN.PurchasedState
  connectionState: BraveVPN.ConnectionState
  selectedRegion: BraveVPN.Region
  initialized: boolean
}

const defaultState: BraveVPNState = {
  purchasedState: BraveVPN.PurchasedState.NOT_PURCHASED,
  connectionState: BraveVPN.ConnectionState.DISCONNECTED,
  selectedRegion: new BraveVPN.Region(),
  initialized: false
}

const reducer = createReducer<BraveVPNState>({}, defaultState)

reducer.on(Actions.connectionStateChanged, (state, payload): BraveVPNState => {
  return {
    ...state,
    connectionState: payload
  }
})

reducer.on(Actions.purchasedStateChanged, (state, payload): BraveVPNState => {
  // Don't update if it's in-progress to prevent unnecessary vpn card page chanage.
  // Set initialized when final state is received once.
  const isLoading = payload === BraveVPN.PurchasedState.LOADING

  return {
    ...state,
    purchasedState:
      isLoading
        ? state.purchasedState
        : payload,
    initialized: isLoading ? state.initialized : true
  }
})

reducer.on(Actions.selectedRegionChanged, (state, payload): BraveVPNState => {
  return {
    ...state,
    selectedRegion: payload
  }
})

export default reducer
