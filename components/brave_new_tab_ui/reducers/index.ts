// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { combineReducers } from 'redux'
import * as storage from '../storage/new_tab_storage'

// Reducers
import newTabReducer from './new_tab_reducer'
import gridSitesReducer from './grid_sites_reducer'
import binanceReducer from './binance_reducer'

const newTabReducers = (state: NewTab.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state
  state = newTabReducer(state, action)
  state = binanceReducer(state, action)

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default combineReducers<NewTab.ApplicationState>({
  newTabData: newTabReducers,
  gridSitesData: gridSitesReducer
})
