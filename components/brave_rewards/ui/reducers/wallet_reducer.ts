/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'

const walletReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.CREATE_WALLET:
      chrome.send('createWalletRequested', [])
      break
    case types.WALLET_CREATED:
      state = { ...state }
      state.walletCreated = true
      state.enabledMain = true
      state.enabledAds = true
      state.enabledContribute = true
      state.createdTimestamp = new Date().getTime()
      chrome.send('getReconcileStamp', [])
      break
    case types.WALLET_CREATE_FAILED:
      state = { ...state }
      state.walletCreateFailed = true
      break
    case types.GET_WALLET_PROPERTIES:
      chrome.send('getWalletProperties', [])
      break
    case types.ON_WALLET_PROPERTIES:
      {
        state = { ...state }
        let ui = state.ui

        // TODO NZ check why enum can't be used inside Rewards namespace
        if (action.payload.properties.status === 1) {
          ui.walletServerProblem = true
        } else {
          // TODO NZ don't just assign directly
          state.walletInfo = action.payload.properties.wallet
          ui.walletServerProblem = false
        }

        state = {
          ...state,
          ui
        }
        break
      }
    case types.GET_WALLLET_PASSPHRASE:
      chrome.send('getWalletPassphrase', [])
      break
    case types.ON_WALLLET_PASSPHRASE:
      const value = action.payload.pass
      if (value && value.length > 0) {
        state = { ...state }
        state.recoveryKey = value
      }
      break
    case types.RECOVER_WALLET:
      if (!action.payload.key || action.payload.key.length === 0) {
        let ui = state.ui
        ui.walletRecoverySuccess = false

        state = {
          ...state,
          ui
        }

        break
      }

      chrome.send('recoverWallet', [action.payload.key])
      break
    case types.ON_RECOVER_WALLET_DATA:
      {
        state = { ...state }
        const result = action.payload.properties.result
        const balance = action.payload.properties.balance
        const grants = action.payload.properties.grants
        let ui = state.ui
        let walletInfo = state.walletInfo

        // TODO NZ check why enum can't be used inside Rewards namespace
        ui.walletRecoverySuccess = result === 0
        if (result === 0) {
          walletInfo.balance = balance
          walletInfo.grants = grants || []
          chrome.send('getWalletPassphrase', [])
          ui.emptyWallet = balance <= 0
          ui.modalBackup = false
        }

        state = {
          ...state,
          ui,
          walletInfo
        }
        break
      }
  }

  return state
}

export default walletReducer
