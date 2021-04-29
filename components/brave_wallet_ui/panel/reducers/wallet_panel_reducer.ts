/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { createReducer } from 'redux-act'
import * as Actions from '../actions/wallet_panel_actions'
import { WalletPanelReducerState } from '../../constants/types'
import { InitializedPayloadType } from '../constants/action_types'

const defaultState: WalletPanelReducerState = {
  hasInitialized: false,
  // TODO(bbondy): isConnected, connectedSiteOrigin, and accounts is just test
  // data to start with until the keyring controller is ready.
  isConnected: false,
  connectedSiteOrigin: 'https://app.uniswap.org',
  accounts: [{
    id: '1',
    name: 'Account 1',
    address: '0x7d66cD_INVALID-ADDRESS_332f3Cd06C',
    balance: 0.31178,
    asset: 'eth'
  }, {
    id: '2',
    name: 'Account 2',
    address: '0x73A29A1_INVALID-ADDRESS_F8895bDCf',
    balance: 0.31178,
    asset: 'eth'
  }, {
    id: '3',
    name: 'Account 3',
    address: '0x3f29A1da_INVALID-ADDRESS_7895b426',
    balance: 0.31178,
    asset: 'eth'
  }]
}

const reducer = createReducer<WalletPanelReducerState>({}, defaultState)

reducer.on(Actions.initialized, (state: WalletPanelReducerState, payload: InitializedPayloadType) => {
  return {
    ...state,
    hasInitialized: true,
    isConnected: payload.isConnected
  }
})

export default reducer
