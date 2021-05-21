/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { createReducer } from 'redux-act'
import * as Actions from '../actions/wallet_page_actions'
import { PageState } from '../../constants/types'
import { WalletCreatedPayloadType } from '../constants/action_types'

const defaultState: PageState = {
  hasInitialized: false
}

const reducer = createReducer<PageState>({}, defaultState)

reducer.on(Actions.walletCreated, (state: PageState, payload: WalletCreatedPayloadType) => {
  return {
    ...state,
    mnemonic: payload.mnemonic
  }
})

reducer.on(Actions.walletSetupComplete, (state: PageState) => {
  delete state['mnemonic']
  return {
    ...state
  }
})

export default reducer
