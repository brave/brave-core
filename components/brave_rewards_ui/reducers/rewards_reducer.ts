/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'

// Utils
import * as storage from '../storage'

const rewardsReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State | undefined, action) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state
  switch (action.type) {
    case types.CREATE_WALLET_REQUESTED:
      chrome.send('createWalletRequested', [])
      break
    case types.WALLET_CREATED:
      state = { ...state }
      state.walletCreated = true
      break
    case types.WALLET_CREATE_FAILED:
      state = { ...state }
      state.walletCreateFailed = true
      break
    case types.GET_WALLET_PROPERTIES:
      chrome.send('getWalletProperties', [])
      break
    case types.ON_WALLET_PROPERTIES:
      state = { ...state }
      console.log(chrome.getVariableValue('rewards.walletInfo.probi'))
      console.log(chrome.getVariableValue('rewards.walletInfo.rates.USD'))
      console.log(chrome.getVariableValue('rewards.walletInfo.rates.EUR'))
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default rewardsReducer
