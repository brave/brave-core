// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Redux API
import { createStore, applyMiddleware } from 'redux'

// Feature core reducer
import { mainNewTabReducer } from './reducers'
import braveVPNAsyncHandler from './async/brave_vpn'
// <if expr="enable_brave_news">
import todayAsyncHandler from './async/today'
// </if>

const middleware = [
// <if expr="enable_brave_news">
  todayAsyncHandler,
// </if>
  braveVPNAsyncHandler
]

export default createStore(
  mainNewTabReducer,
  applyMiddleware(...middleware)
)
