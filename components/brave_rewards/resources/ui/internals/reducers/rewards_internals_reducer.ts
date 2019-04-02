/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/rewards_internals_types'

// Utils
import * as storage from '../storage'

const rewardsInternalsReducer: Reducer<RewardsInternals.State | undefined> = (state: RewardsInternals.State | undefined, action) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state
  switch (action.type) {
    case types.GET_REWARDS_ENABLED:
      chrome.send('brave_rewards_internals.getRewardsEnabled')
      break
    case types.GET_REWARDS_INTERNALS_INFO:
      chrome.send('brave_rewards_internals.getRewardsInternalsInfo')
      break
    case types.ON_GET_REWARDS_ENABLED:
      state = { ...state }
      state.isRewardsEnabled = action.payload.enabled
      break
    case types.ON_GET_REWARDS_INTERNALS_INFO:
      state = { ...state }
      state.info = action.payload.info
      break
    case types.ON_RESET:
      state = { ...storage.defaultState }
      break
    default:
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default rewardsInternalsReducer
