/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/donate_types'

export const defaultState: RewardsDonate.State = {
  finished: false,
  error: false,
  publishers: {},
  currentTipAmount: '0.0',
  currentTipRecurring: false,
  recurringDonations: [],
  walletInfo: {
    balance: 0,
    choices: [],
    probi: '0'
  },
  reconcileStamp: 0
}

const publishersReducer: Reducer<RewardsDonate.State> = (state: RewardsDonate.State = defaultState, action) => {
  const payload = action.payload

  switch (action.type) {
    case types.ON_CLOSE_DIALOG:
      state = { ...state }
      state.currentTipRecurring = false
      chrome.send('dialogClose')
      break
    case types.ON_PUBLISHER_BANNER: {
      state = { ...state }
      if (!state.publishers) {
        state.publishers = {}
      }
      const publisher: RewardsDonate.Publisher = payload.data
      state.publishers[publisher.publisherKey] = publisher
      break
    }
    case types.GET_WALLET_PROPERTIES:
      chrome.send('brave_rewards_donate.getWalletProperties')
      break
    case types.ON_WALLET_PROPERTIES: {
      state = { ...state }
      if (payload.properties.status !== 1) {
        state.walletInfo = payload.properties.wallet
      }
      break
    }
    case types.ON_DONATE: {
      if (payload.publisherKey && payload.amount > 0) {
        let amount = parseInt(payload.amount, 10)
        chrome.send('brave_rewards_donate.onDonate', [
          payload.publisherKey,
          amount,
          payload.recurring
        ])
        state = { ...state }
        state.finished = true
        state.currentTipAmount = amount.toFixed(1)
        state.currentTipRecurring = payload.recurring
      } else {
        // TODO return error
      }
      break
    }
    case types.GET_RECURRING_TIPS:
    case types.ON_RECURRING_TIP_REMOVED:
      chrome.send('brave_rewards_donate.getRecurringTips')
      break
    case types.ON_RECURRING_TIPS:
      state = { ...state }
      const tips = action.payload.list

      if (tips) {
        state.recurringDonations = tips
      }
      break
    case types.GET_RECONCILE_STAMP: {
      chrome.send('brave_rewards_donate.getReconcileStamp')
      break
    }
    case types.ON_RECONCILE_STAMP: {
      state = { ...state }
      state.reconcileStamp = action.payload.stamp
      break
    }
  }

  return state
}

export default publishersReducer
