/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'
import { defaultState } from '../storage'

const rewardsReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.ON_SETTING_SAVE:
      state = { ...state }
      const key = action.payload.key
      const value = action.payload.value
      if (key) {
        state[key] = value
        chrome.send('brave_rewards.saveSetting', [key, value.toString()])
      }
      break
    case types.ON_MODAL_BACKUP_CLOSE: {
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
    case types.ON_MODAL_BACKUP_OPEN: {
      let ui = state.ui
      ui.modalBackup = true
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_CLEAR_ALERT: {
      let ui = state.ui
      if (!ui[action.payload.property]) {
        break
      }

      ui[action.payload.property] = null
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_RECONCILE_STAMP: {
      state = { ...state }
      state.reconcileStamp = parseInt(action.payload.stamp, 10)
      break
    }
    case types.GET_ADS_DATA: {
      chrome.send('brave_rewards.getAdsData')
      break
    }
    case types.ON_ADS_DATA: {
      if (!action.payload.adsData) {
        break
      }

      state = {
        ...state,
        adsData: {
          ...state.adsData,
          ...action.payload.adsData
        }
      }
      break
    }
    case types.ON_ADS_SETTING_SAVE: {
      state = { ...state }
      const key = action.payload.key
      const value = action.payload.value
      if (key) {
        state[key] = value
        chrome.send('brave_rewards.saveAdsSetting', [key, value.toString()])
      }
      break
    }
    case types.GET_TRANSACTION_HISTORY:
    case types.ON_TRANSACTION_HISTORY_CHANGED: {
      chrome.send('brave_rewards.getTransactionHistory', [])
      break
    }
    case types.ON_TRANSACTION_HISTORY: {
      if (!action.payload.data) {
        break
      }

      state = { ...state }

      if (!state.adsData) {
        state.adsData = defaultState.adsData
      }

      const data = action.payload.data
      state.adsData.adsEstimatedPendingRewards = data.adsEstimatedPendingRewards
      state.adsData.adsNextPaymentDate = data.adsNextPaymentDate
      state.adsData.adsAdNotificationsReceivedThisMonth = data.adsAdNotificationsReceivedThisMonth
      break
    }
    case types.INIT_AUTOCONTRIBUTE_SETTINGS: {
      state = { ...state }
      let properties = action.payload.properties
      let ui = state.ui

      if (!properties || Object.keys(properties).length === 0) {
        break
      }

      Object.keys(properties).map((property: string) => {
        if (properties[property] !== undefined && properties[property] !== 'ui') {
          state[property] = properties[property]
        } else if (properties[property] === 'ui') {
          ui = Object.assign(ui, properties[property])
        }
      })

      state = {
        ...state,
        ui
      }

      break
    }
    case types.GET_DONATION_TABLE: {
      chrome.send('brave_rewards.getRecurringTips')
      chrome.send('brave_rewards.getOneTimeTips')
      break
    }
    case types.ON_REWARDS_ENABLED: {
      state = { ...state }
      state.enabledMain = action.payload.enabled
      break
    }
    case types.GET_REWARDS_ENABLED: {
      chrome.send('brave_rewards.getRewardsMainEnabled', [])
      break
    }
    case types.ONLY_ANON_WALLET: {
      chrome.send('brave_rewards.onlyAnonWallet')
      break
    }
    case types.ON_ONLY_ANON_WALLET: {
      const ui = state.ui

      ui.onlyAnonWallet = !!action.payload.only

      state = {
        ...state,
        ui
      }
      break
    }
  }

  return state
}

export default rewardsReducer
