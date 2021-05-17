/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { createReducer } from 'redux-act'
import { WalletState } from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'
import { InitializedPayloadType } from '../constants/action_types'

const defaultState: WalletState = {
  hasInitialized: false,
  isWalletCreated: false,
  isWalletLocked: true,
  isWalletRecoveryVerified: false
}

const reducer = createReducer<WalletState>({}, defaultState)

reducer.on(WalletActions.initialized, (state: any, payload: InitializedPayloadType) => {
  return {
    ...state,
    hasInitialized: true,
    isWalletCreated: payload.isWalletCreated
  }
})

export default reducer
