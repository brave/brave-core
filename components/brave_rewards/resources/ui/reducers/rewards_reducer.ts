/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'
import { defaultState } from '../storage'

const rewardsReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.INIT_AUTOCONTRIBUTE_SETTINGS: {
      state = { ...state }
      let properties = action.payload.properties
      let ui = state.ui

      if (!properties || Object.keys(properties).length === 0) {
        break
      }

      const isEmpty = !ui || ui.emptyWallet

      Object.keys(properties).map((property: string) => {
        if (properties[property] !== undefined && properties[property] !== 'ui') {
          state[property] = properties[property]
        } else if (properties[property] === 'ui') {
          ui = Object.assign(ui, properties[property])
        }
      })

      if (!isEmpty) {
        ui.emptyWallet = false
      }

      state = {
        ...state,
        ui
      }

      break
    }
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
    case types.GET_TIP_TABLE: {
      chrome.send('brave_rewards.getRecurringTips')
      chrome.send('brave_rewards.getOneTimeTips')
      break
    }
    case types.GET_CONTRIBUTE_LIST: {
      chrome.send('brave_rewards.getContributionList')
      break
    }
    case types.CHECK_IMPORTED: {
      chrome.send('brave_rewards.checkImported')
      break
    }
    case types.ON_IMPORTED_CHECK: {
      let ui = state.ui
      ui.walletImported = action.payload.imported
      state = {
        ...state,
        ui
      }
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

      state = { ...state }

      if (!state.adsData) {
        state.adsData = defaultState.adsData
      }

      state.adsData.adsEnabled = action.payload.adsData.adsEnabled
      state.adsData.adsPerHour = action.payload.adsData.adsPerHour
      state.adsData.adsUIEnabled = action.payload.adsData.adsUIEnabled
      state.adsData.adsIsSupported = action.payload.adsData.adsIsSupported
      break
    }
    case types.ON_ADS_SETTING_SAVE: {
      state = { ...state }
      const key = action.payload.key
      const value = action.payload.value
      if (key) {
        state[key] = value
        chrome.send('brave_rewards.saveAdsSetting', [key, value.toString()])

        if (key === 'adsEnabledMigrated') {
          state.enabledAdsMigrated = true
        }
      }
      break
    }
    case types.ON_REWARDS_ENABLED: {
      state = { ...state }
      state.enabledMain = action.payload.enabled
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
    case types.GET_REWARDS_MAIN_ENABLED: {
      chrome.send('brave_rewards.getRewardsMainEnabled', [])
      break
    }
    case types.ON_CONTRIBUTION_SAVED: {
      const properties = action.payload.properties
      console.log(properties)
      if (properties && properties.success && properties.category === 8) {
        chrome.send('brave_rewards.getOneTimeTips')
      }
      break
    }
    case types.ON_INLINE_TIP_SETTINGS_CHANGE: {
      if (!state.inlineTip) {
        state.inlineTip = {
          twitter: true
        }
      }

      const key = action.payload.key
      const value = action.payload.value

      if (key == null || key.length === 0 || value == null) {
        break
      }

      let inlineTip = state.inlineTip

      inlineTip[key] = value
      chrome.send('brave_rewards.setInlineTipSetting', [key, value.toString()])

      state = {
        ...state,
        inlineTip
      }

      break
    }
  }

  return state
}

export default rewardsReducer
