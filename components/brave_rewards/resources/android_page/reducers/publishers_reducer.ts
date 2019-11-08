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
    case types.ON_EXCLUDED_LIST:
      if (!action.payload.list) {
        break
      }

      state = { ...state }
      state.excludedList = action.payload.list
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
    case types.GET_CONTRIBUTE_LIST:
      chrome.send('brave_rewards.getContributionList')
      break
    case types.ON_CURRENT_TIPS:
      state = { ...state }
      if (state.tipsLoad) {
        state.firstLoad = false
      } else {
        state.tipsLoad = true
      }
      state.tipsList = action.payload.list
      break
    case types.ON_RECURRING_DONATION_UPDATE:
      state = { ...state }
      if (state.recurringLoad) {
        state.firstLoad = false
      } else {
        state.recurringLoad = true
      }
      state.recurringList = action.payload.list
      break
    case types.ON_REMOVE_RECURRING:
      if (!action.payload.publisherKey) {
        break
      }
      chrome.send('brave_rewards.removeRecurringTip', [action.payload.publisherKey])
      break
    case types.GET_EXCLUDED_SITES:
      chrome.send('brave_rewards.getExcludedSites')
      break
    case types.ON_RECURRING_TIP_REMOVED:
      chrome.send('brave_rewards.getRecurringTips')
      break
  }

  return state
}

export default publishersReducer
