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
      state = { ...state }
      // TODO NZ don't just assign directly
      state.walletInfo = action.payload.properties
      break
    case types.GET_PROMOTION:
      chrome.send('getPromotion', [])
      break
    case types.ON_PROMOTION:
      state = { ...state }
      state.promotion = {
        amount: action.payload.properties.amount,
        promotionId: action.payload.properties.promotionId
      }
      break
    case types.GET_PROMOTION_CAPTCHA:
      chrome.send('getPromotionCaptcha', [])
      break
    case types.ON_PROMOTION_CAPTCHA:
      {
        if (state.promotion) {
          let promotion = state.promotion
          promotion.captcha = `data:image/jpeg;base64,${action.payload.image}`
          state = {
            ...state,
            promotion
          }
        }

        break
      }
    case types.SOLVE_PROMOTION_CAPTCHA:
      if (action.payload.x && action.payload.y) {
        chrome.send('solvePromotionCaptcha', [JSON.stringify({
          x: action.payload.x,
          y: action.payload.y
        })])
      }
      break
    case types.ON_PROMOTION_RESET:
      {
        if (state.promotion) {
          let promotion: Rewards.Promotion = {
            promotionId: state.promotion.promotionId,
            amount: state.promotion.amount
          }

          state = {
            ...state,
            promotion
          }
        }

        break
      }
    case types.ON_PROMOTION_DELETE:
      {
        if (state.promotion) {
          delete state.promotion

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
        let ui = state.ui
        let walletInfo = state.walletInfo

        // TODO NZ check why enum can't be used inside Rewards namespace
        ui.walletRecoverySuccess = result === 0
        if (result === 0) {
          walletInfo.balance = balance
          chrome.send('getWalletPassphrase', [])

          if (balance > 0) {
            ui.emptyWallet = false
          }
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
        state = {
          ...state,
          ui
        }
        break
      }
    case types.ON_PROMOTION_FINISH:
      state = { ...state }
      const properties = action.payload.properties
      // TODO NZ check why enum can't be used inside Rewards namespace
      if (properties.result === 0) {
        if (state.promotion) {
          let promotion = state.promotion
          promotion.expireDate = properties.expirationDate
          promotion.error = null

          state = {
            ...state,
            promotion
          }
          chrome.send('getWalletProperties', [])
        }
      } else {
        state = { ...state }
        if (state.promotion) {
          let promotion = state.promotion
          promotion.error = 'wrongPosition'

          state = {
            ...state,
            promotion
          }
        }
        chrome.send('getPromotionCaptcha', [])
      }
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default rewardsReducer
