/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'
import { defaultState } from '../storage'

const rewardsReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.IS_INITIALIZED: {
      chrome.send('brave_rewards.isInitialized')
      break
    }
    case types.GET_AUTO_CONTRIBUTE_PROPERTIES: {
      chrome.send('brave_rewards.getAutoContributeProperties')
      break
    }
    case types.DISCONNECT_WALLET : {
      state = { ...state }
      chrome.send('brave_rewards.disconnectWallet')
      break
    }
    case types.DISCONNECT_WALLET_ERROR: {
      state = { ...state }
      let ui = state.ui
      ui.disconnectWalletError = true
      break
    }
    case types.ON_AUTO_CONTRIBUTE_PROPERTIES: {
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
    case types.GET_EXTERNAL_WALLET: {
      chrome.send('brave_rewards.getExternalWallet')
      break
    }
    case types.ON_EXTERNAL_WALLET: {
      state = { ...state }

      if (action.payload.result === 24) { // on ledger::type::Result::EXPIRED_TOKEN
        chrome.send('brave_rewards.getExternalWallet')
        break
      }

      state.externalWallet = action.payload.wallet
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
      ui.walletRecoveryStatus = null
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
      state.adsData.adsSubdivisionTargeting = action.payload.adsData.adsSubdivisionTargeting
      state.adsData.automaticallyDetectedAdsSubdivisionTargeting = action.payload.adsData.automaticallyDetectedAdsSubdivisionTargeting
      state.adsData.shouldAllowAdsSubdivisionTargeting = action.payload.adsData.shouldAllowAdsSubdivisionTargeting
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
    case types.GET_STATEMENT:
    case types.ON_STATEMENT_CHANGED: {
      chrome.send('brave_rewards.getStatement', [])
      break
    }
    case types.ON_STATEMENT: {
      if (!action.payload.data) {
        break
      }

      state = { ...state }

      if (!state.adsData) {
        state.adsData = defaultState.adsData
      }

      const data = action.payload.data
      state.adsData.adsNextPaymentDate = data.adsNextPaymentDate
      state.adsData.adsReceivedThisMonth = data.adsReceivedThisMonth
      state.adsData.adsEarningsThisMonth = data.adsEarningsThisMonth
      state.adsData.adsEarningsLastMonth = data.adsEarningsLastMonth
      break
    }
    case types.ON_INLINE_TIP_SETTINGS_CHANGE: {
      if (!state.inlineTip) {
        state.inlineTip = {
          twitter: true,
          reddit: true,
          github: true
        }
      }

      const key = action.payload.key
      const value = action.payload.value

      if (key == null || key.length === 0 || value == null) {
        break
      }

      let inlineTip = state.inlineTip

      inlineTip[key] = value
      chrome.send('brave_rewards.setInlineTippingPlatformEnabled', [key, value.toString()])

      state = {
        ...state,
        inlineTip
      }

      break
    }
    case types.ON_INITIALIZED: {
      state = {
        ...state,
        initializing: false
      }
      chrome.send('brave_rewards.getReconcileStamp')
      break
    }
  }

  return state
}

export default rewardsReducer
