// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { createReducer } from 'redux-act'
import * as Actions from '../../actions/today_actions'

const defaultState: NewTab.BraveTodayState = {
  isLoaded: false,
  isLoading: true
}

const reducer = createReducer<NewTab.BraveTodayState>({}, defaultState)

export default reducer

reducer.on(Actions.init, (state, payload) => ({
  ...state,
  isLoading: true
}))

