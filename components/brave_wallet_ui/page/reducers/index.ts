/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { combineReducers } from 'redux'

import pageReducer from './page_reducer'
import walletReducer from '../../common/slices/wallet.slice'
import sendCryptoReducer from '../../common/reducers/send_crypto_reducer'
import accountsTabReducer from './accounts-tab-reducer'

export const walletPageReducer = combineReducers({
  page: pageReducer,
  wallet: walletReducer,
  sendCrypto: sendCryptoReducer,
  accountsTab: accountsTabReducer
})

export default walletPageReducer
