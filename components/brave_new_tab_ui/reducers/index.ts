// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { combineReducers } from 'redux'
import * as storage from '../storage/new_tab_storage'

// Reducers
import newTabStateReducer from './new_tab_reducer'
import gridSitesReducer from './grid_sites_reducer'
import binanceReducer from './binance_reducer'
import rewardsReducer from './rewards_reducer'
import geminiReducer from './gemini_reducer'
import cryptoDotComReducer from './cryptoDotCom_reducer'
import ftxReducer from '../widgets/ftx/ftx_reducer'
import { FTXState } from '../widgets/ftx/ftx_state'
import { stackWidgetReducer } from './stack_widget_reducer'
import todayReducer, { BraveTodayState } from './today'

export type ApplicationState = NewTab.ApplicationState & {
  today: BraveTodayState
  ftx: FTXState
}

export const newTabReducers = (state: NewTab.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state
  state = newTabStateReducer(state, action)
  state = binanceReducer(state, action)
  state = rewardsReducer(state, action)
  state = geminiReducer(state, action)
  state = cryptoDotComReducer(state, action)
  state = stackWidgetReducer(state, action)

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export const mainNewTabReducer = combineReducers<ApplicationState>({
  newTabData: newTabReducers,
  gridSitesData: gridSitesReducer,
  today: todayReducer,
  ftx: ftxReducer
})

export const newTabReducer = newTabStateReducer
