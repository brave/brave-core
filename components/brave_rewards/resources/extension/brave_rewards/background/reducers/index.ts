/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { combineReducers } from 'redux'
import * as storage from '../storage'
import { setBadgeText } from '../browserAction'

// Utils
import { promotionPanelReducer } from './promotion_reducer'
import { rewardsPanelReducer } from './rewards_panel_reducer'

const mergeReducers = (state: RewardsExtension.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
    setBadgeText(state)
  }

  state = rewardsPanelReducer(state, action)
  state = promotionPanelReducer(state, action)

  if (!state) {
    state = storage.defaultState
  }

  return state
}

export default combineReducers<RewardsExtension.ApplicationState>({
  rewardsPanelData: mergeReducers
})
