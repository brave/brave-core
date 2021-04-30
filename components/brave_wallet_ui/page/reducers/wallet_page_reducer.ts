/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { createReducer } from 'redux-act'
import * as Actions from '../actions/wallet_page_actions'
import { WalletPageReducerState } from '../../constants/types'
import { InitializedPayloadType } from '../constants/action_types'

const defaultState: WalletPageReducerState = {
  hasInitialized: false,
}

const reducer = createReducer<WalletPageReducerState>({}, defaultState)

reducer.on(Actions.initialized, (state: WalletPageReducerState, payload: InitializedPayloadType) => {
  return {
    ...state,
    hasInitialized: true,
    isConnected: payload.isConnected
  }
})

export default reducer
