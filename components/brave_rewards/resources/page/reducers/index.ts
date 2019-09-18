/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { combineReducers } from 'redux'

// Utils
import rewardsReducer from './rewards_reducer'
import walletReducer from './wallet_reducer'
import grantReducer from './grant_reducer'
import publishersReducer from './publishers_reducer'
import * as storage from '../storage'

const mergeReducers = (state: Rewards.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }
  const startingState = state

  state = rewardsReducer(state, action)
  state = walletReducer(state, action)
  state = grantReducer(state, action)
  state = publishersReducer(state, action)

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default combineReducers<Rewards.ApplicationState>({
  rewardsData: mergeReducers
})
