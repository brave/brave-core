/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'

const publishersReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  if (!state) {
    return
  }

  switch (action.type) {
    case types.ON_CONTRIBUTE_LIST:
      state = { ...state }
      state.autoContributeList = action.payload.list
      break
    case types.ON_EXCLUDED_LIST: {
      if (!action.payload.list) {
        break
      }

      state = { ...state }
      state.excludedList = action.payload.list
      break
    }
    case types.ON_EXCLUDE_PUBLISHER: {
      const publisherKey: string = action.payload.publisherKey
      if (!publisherKey) {
        break
      }

      chrome.send('brave_rewards.excludePublisher', [publisherKey])
      break
    }
    case types.ON_RESTORE_PUBLISHER: {
      const publisherKey: string = action.payload.publisherKey
      if (!publisherKey) {
        break
      }

      chrome.send('brave_rewards.restorePublisher', [publisherKey])
      break
    }
    case types.ON_RESTORE_PUBLISHERS:
      state = { ...state }
      chrome.send('brave_rewards.restorePublishers', [])
      break
    case types.ON_RECURRING_TIPS:
      state = { ...state }
      state.recurringList = action.payload.list || []
      break
    case types.REMOVE_RECURRING_TIP:
      if (!action.payload.publisherKey) {
        break
      }
      chrome.send('brave_rewards.removeRecurringTip', [action.payload.publisherKey])
      break
    case types.ON_CURRENT_TIPS:
      state = { ...state }
      state.tipsList = action.payload.list || []
      break
    case types.ON_RECURRING_TIP_SAVED:
    case types.ON_RECURRING_TIP_REMOVED:
      chrome.send('brave_rewards.getRecurringTips')
      break
    case types.GET_EXCLUDED_SITES:
      chrome.send('brave_rewards.getExcludedSites')
      break
    case types.ON_RECONCILE_STAMP_RESET:
      chrome.send('brave_rewards.getContributionList')
      break
  }

  return state
}

export default publishersReducer
