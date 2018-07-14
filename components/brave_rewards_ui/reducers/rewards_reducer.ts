/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global chrome */

const types = require('../constants/rewards_types')
const storage = require('../storage')

const rewardsReducer = (state, action) => {
  if (state === undefined) {
    state = storage.load() || {}
    state = Object.assign(storage.getInitialState(), state)
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
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default rewardsReducer
