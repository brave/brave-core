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
      if (state.contributeLoad) {
        state.firstLoad = false
      } else {
        state.contributeLoad = true
      }

      state.autoContributeList = action.payload.list
      break
    case types.ON_EXCLUDED_PUBLISHERS_NUMBER: {
      state = { ...state }
      let num = parseInt(action.payload.num, 10)

      if (isNaN(num)) {
        num = 0
      }

      state.excludedPublishersNumber = num

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
    case types.ON_RESTORE_PUBLISHERS:
      state = { ...state }
      chrome.send('brave_rewards.restorePublishers', [])
      break
    case types.ON_RECURRING_TIPS:
      state = { ...state }
      if (state.recurringLoad) {
        state.firstLoad = false
      } else {
        state.recurringLoad = true
      }
      state.recurringList = action.payload.list
      break
    case types.REMOVE_RECURRING_TIP:
      if (!action.payload.publisherKey) {
        break
      }
      chrome.send('brave_rewards.removeRecurringTip', [action.payload.publisherKey])
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
    case types.GET_EXCLUDED_PUBLISHERS_NUMBER:
      chrome.send('brave_rewards.getExcludedPublishersNumber')
      break
    case types.ON_RECURRING_TIP_SAVED:
    case types.ON_RECURRING_TIP_REMOVED:
      chrome.send('brave_rewards.getRecurringTips')
      break
  }

  return state
}

export default publishersReducer
