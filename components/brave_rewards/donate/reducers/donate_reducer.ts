/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/donate_types'

export const defaultState: RewardsDonate.State = {
  finished: false,
  error: false,
  publisher: undefined,
  walletInfo: {
    balance: 0,
    choices: [],
    probi: '0'
  }
}

const publishersReducer: Reducer<RewardsDonate.State> = (state: RewardsDonate.State = defaultState, action) => {
  const payload = action.payload

  switch (action.type) {
    case types.ON_CLOSE_DIALOG:
      chrome.send('dialogClose')
      break
    case types.ON_PUBLISHER_BANNER:
      {
        state = { ...state }
        state.publisher = payload.data
        break
      }
    case types.GET_WALLET_PROPERTIES:
      chrome.send('brave_rewards_donate.getWalletProperties')
      break
    case types.ON_WALLET_PROPERTIES:
      {
        state = { ...state }
        if (payload.properties.status !== 1) {
          state.walletInfo = payload.properties.wallet
        }
        break
      }
    case types.ON_DONATE:
      {
        if (state.publisher && state.publisher.publisherKey && payload.amount > 0) {
          chrome.send('brave_rewards_donate.onDonate', [
            payload.publisherKey,
            parseInt(payload.amount, 10),
            payload.recurring
          ])
          state = { ...state }
          state.finished = true;
        } else {
          // TODO return error
        }
        break
      }
  }

  return state
}

export default publishersReducer
