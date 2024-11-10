// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { combineReducers } from 'redux'
import * as storage from '../storage/new_tab_storage'

// Reducers
import newTabStateReducer from './new_tab_reducer'
import gridSitesReducer from './grid_sites_reducer'
import rewardsReducer from './rewards_reducer'
import { stackWidgetReducer } from './stack_widget_reducer'
import todayReducer, { BraveNewsState } from './today'
import braveVPNReducer, { BraveVPNState } from './brave_vpn'

export type ApplicationState = NewTab.ApplicationState & {
  today: BraveNewsState
  braveVPN: BraveVPNState
}

export const newTabReducers = (state: NewTab.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state
  state = newTabStateReducer(state, action)
  state = rewardsReducer(state, action)
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
  braveVPN: braveVPNReducer
})

export const newTabReducer = newTabStateReducer
