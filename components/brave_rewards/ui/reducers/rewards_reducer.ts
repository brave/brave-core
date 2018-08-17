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
    case types.ON_SETTING_SAVE:
      state = { ...state }
      if (action.payload.key) {
        state[action.payload.key] = action.payload.value
      }
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
    case types.GET_PROMOTION:
      chrome.send('getGrant', [])
      break
    case types.ON_GRANT:
      state = { ...state }
      // TODO NZ check why enum can't be used inside Rewards namespace
      if (action.payload.properties.status === 1) {
        state.grant = undefined
        break
      }

      state.grant = {
        promotionId: action.payload.properties.promotionId,
        expiryTime: 0,
        probi: ''
      }
      break
    case types.GET_PROMOTION_CAPTCHA:
      chrome.send('getGrantCaptcha', [])
      break
    case types.ON_GRANT_CAPTCHA:
      {
        if (state.grant) {
          let grant = state.grant
          grant.captcha = `data:image/jpeg;base64,${action.payload.image}`
          state = {
            ...state,
            grant
          }
        }

        break
      }
    case types.SOLVE_PROMOTION_CAPTCHA:
      if (action.payload.x && action.payload.y) {
        chrome.send('solveGrantCaptcha', [JSON.stringify({
          x: action.payload.x,
          y: action.payload.y
        })])
      }
      break
    case types.ON_GRANT_RESET:
      {
        if (state.grant) {
          const grant: Rewards.Grant = {
            promotionId: state.grant.promotionId,
            probi: '',
            expiryTime: 0
          }

          state = {
            ...state,
            grant
          }
        }

        break
      }
    case types.ON_GRANT_DELETE:
      {
        if (state.grant) {
          delete state.grant

          state = {
            ...state
          }
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
    case types.ON_MODAL_BACKUP_CLOSE:
      {
        state = { ...state }
        let ui = state.ui
        ui.walletRecoverySuccess = null
        ui.modalBackup = false
        state = {
          ...state,
          ui
        }
        break
      }
    case types.ON_GRANT_FINISH:
      {
        state = { ...state }
        const properties = action.payload.properties
        // TODO NZ check why enum can't be used inside Rewards namespace
        if (properties.result === 0) {
          if (state.grant) {
            let grant = state.grant
            let ui = state.ui
            grant.expiryTime = properties.expiryTime * 1000
            grant.status = null
            ui.emptyWallet = false

            state = {
              ...state,
              grant,
              ui
            }
            chrome.send('getWalletProperties', [])
          }
        } else {
          state = { ...state }
          if (state.grant) {
            let grant = state.grant
            grant.status = 'wrongPosition'

            state = {
              ...state,
              grant
            }
          }
          chrome.send('getGrantCaptcha', [])
        }
        break
      }
    case types.ON_MODAL_BACKUP_OPEN:
      {
        let ui = state.ui
        ui.modalBackup = true
        state = {
          ...state,
          ui
        }
        break
      }
    case types.ON_CLEAR_ALERT:
      {
        let ui = state.ui
        ui[action.payload.error] = null
        state = {
          ...state,
          ui
        }
        break
      }
    case types.ON_RECONCILE_STAMP:
      {

        state = { ...state }
        state.reconcileStamp = parseInt(action.payload.stamp, 10)
        break
      }
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default rewardsReducer
