/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'

const publishersReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.ON_CONTRIBUTE_LIST:
      state = { ...state }
      state.firstLoad = false
      state.autoContributeList = action.payload.list
      break
    case types.ON_NUM_EXCLUDED_SITES:
      state = { ...state }
      if (action.payload.num != null) {
        state.numExcludedSites = parseInt(action.payload.num, 10)
      }
      break
    case types.ON_EXCLUDE_PUBLISHER:
      if (!action.payload.publisherKey) {
        break
      }
      chrome.send('brave_rewards.excludePublisher', [action.payload.publisherKey])
      break
    case types.ON_RESTORE_PUBLISHERS:
      chrome.send('brave_rewards.restorePublishers', [])
      break
  }

  return state
}

export default publishersReducer
