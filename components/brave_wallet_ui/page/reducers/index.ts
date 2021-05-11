/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { combineReducers } from 'redux'

import pageReducer from './page_reducer'
import walletReducer from '../../common/reducers/wallet_reducer'

export default combineReducers({
  page: pageReducer,
  wallet: walletReducer
})
