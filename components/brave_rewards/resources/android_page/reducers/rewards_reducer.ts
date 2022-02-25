/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'
import { defaultState } from '../storage'

const rewardsReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  if (!state) {
    return
  }

  switch (action.type) {
    case types.IS_INITIALIZED: {
      chrome.send('brave_rewards.isInitialized')
      break
    }
    case types.GET_AUTO_CONTRIBUTE_PROPERTIES: {
      chrome.send('brave_rewards.getAutoContributeProperties')
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
    case types.ON_SETTING_SAVE: {
      state = { ...state }
      const key = action.payload.key
      const value = action.payload.value
      const persist = action.payload.persist
      if (!key) {
        break
      }

      if (persist) {
        chrome.send('brave_rewards.saveSetting', [key, value.toString()])
      }

      state[key] = value
      break
    }
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
      if (ui[action.payload.property] === undefined) {
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
    case types.GET_ADS_HISTORY: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.ON_ADS_HISTORY: {
      if (!action.payload.adsHistory) {
        break
      }

      state = { ...state }

      if (!state.adsHistory) {
        state.adsHistory = defaultState.adsHistory
      }

      state.adsHistory = action.payload.adsHistory
      break
    }
    case types.TOGGLE_AD_THUMB_UP: {
      chrome.send('brave_rewards.toggleAdThumbUp', [action.payload.adContent])
      break
    }
    case types.ON_TOGGLE_AD_THUMB_UP: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_AD_THUMB_DOWN: {
      chrome.send('brave_rewards.toggleAdThumbDown', [action.payload.adContent])
      break
    }
    case types.ON_TOGGLE_AD_THUMB_DOWN: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_AD_OPT_IN: {
      chrome.send('brave_rewards.toggleAdOptIn',
        [action.payload.category, action.payload.optAction])
      break
    }
    case types.ON_TOGGLE_AD_OPT_IN: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_AD_OPT_OUT: {
      chrome.send('brave_rewards.toggleAdOptOut',
        [action.payload.category, action.payload.optAction])
      break
    }
    case types.ON_TOGGLE_AD_OPT_OUT: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_SAVED_AD: {
      chrome.send('brave_rewards.toggleSavedAd', [action.payload.adContent])
      break
    }
    case types.ON_TOGGLE_SAVED_AD: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_FLAGGED_AD: {
      chrome.send('brave_rewards.toggleFlaggedAd', [action.payload.adContent])
      break
    }
    case types.ON_TOGGLE_FLAGGED_AD: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.ON_ADS_SETTING_SAVE: {
      state = { ...state }
      const key = action.payload.key
      const value = action.payload.value
      if (key) {
        chrome.send('brave_rewards.saveAdsSetting', [key, value.toString()])
        if (key === 'adsEnabledMigrated') {
          state.enabledAdsMigrated = true
        } else {
          state.adsData = { ...(state.adsData || defaultState.adsData) }
          state.adsData[key] = value
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
    case types.GET_ENABLED_INLINE_TIPPING_PLATFORMS: {
      chrome.send('brave_rewards.getEnabledInlineTippingPlatforms')
      break
    }
    case types.ON_ENABLED_INLINE_TIPPING_PLATFORMS: {
      const inlineTip = {
        twitter: false,
        reddit: false,
        github: false
      }

      for (const platform of action.payload.platforms) {
        switch (platform) {
          case 'github':
            inlineTip.github = true
            break
          case 'reddit':
            inlineTip.reddit = true
            break
          case 'twitter':
            inlineTip.twitter = true
            break
        }
      }

      state = {
        ...state,
        inlineTip
      }

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
    case types.PROCESS_REWARDS_PAGE_URL: {
      const path = action.payload.path
      const query = action.payload.query
      const ui = state.ui

      chrome.send('brave_rewards.processRewardsPageUrl', [path, query])
      ui.modalRedirect = 'show'

      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_PROCESS_REWARDS_PAGE_URL: {
      const data = action.payload.data
      const ui = state.ui

      chrome.send('brave_rewards.getExternalWallet')

      if (data.result === 9) { // type::Result::NOT_FOUND
        ui.modalRedirect = 'kycRequiredModal'
        break
      }

      if (data.result === 24) { // type::Result::EXPIRED_TOKEN
        ui.modalRedirect = 'error'
        break
      }

      if (data.result === 25) { // type::Result::UPHOLD_BAT_NOT_ALLOWED
        ui.modalRedirect = 'upholdBATNotAllowedModal'
        break
      }

      if (data.result === 36) { // type::Result::DEVICE_LIMIT_REACHED
        ui.modalRedirect = 'deviceLimitReachedModal'
        break
      }

      if (data.result === 37) { // type::Result::MISMATCHED_PROVIDER_ACCOUNTS
        ui.modalRedirect = 'mismatchedProviderAccountsModal'
        break
      }

      if (data.result !== 0) {
        ui.modalRedirect = 'error'
        break
      }

      if (data.walletType === 'uphold' || data.walletType === 'bitflyer' || data.walletType === 'gemini') {
        chrome.send('brave_rewards.fetchBalance')

        if (data.action === 'authorization') {
          const url = data.args.redirect_url
          if (url && url.length > 0) {
            window.open(url, '_self')
          }
          ui.modalRedirect = 'hide'
          break
        }
      }

      ui.modalRedirect = 'error'

      state = {
        ...state,
        ui
      }
      break
    }
    case types.HIDE_REDIRECT_MODAL: {
      const ui = state.ui

      ui.modalRedirect = 'hide'

      state = {
        ...state,
        ui
      }
      break
    }
    case types.DISCONNECT_WALLET: {
      state = { ...state }
      chrome.send('brave_rewards.disconnectWallet')
      break
    }
    case types.DISMISS_PROMO_PROMPT: {
      const ui = state.ui
      const promoKey = action.payload.promo

      if (!ui.promosDismissed) {
        ui.promosDismissed = {}
      }

      ui.promosDismissed[promoKey] = true

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

      if (action.payload.result === 0) { // type::Result::LEDGER_OK
        chrome.send('brave_rewards.fetchBalance')
      }

      state.externalWallet = action.payload.wallet
      break
    }
    case types.GET_COUNTRY_CODE: {
      chrome.send('brave_rewards.getCountryCode')
      break
    }
    case types.ON_COUNTRY_CODE: {
      const { countryCode } = action.payload
      state = {
        ...state,
        currentCountryCode: countryCode
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
    case types.COMPLETE_RESET: {
      chrome.send('brave_rewards.completeReset')
      break
    }
    case types.ON_COMPLETE_RESET: {
      if (action.payload.success) {
        return undefined
      }
      break
    }
    case types.GET_PAYMENT_ID: {
      chrome.send('brave_rewards.getPaymentId')
      break
    }
    case types.ON_PAYMENT_ID: {
      state = {
        ...state,
        paymentId: action.payload.paymentId
      }
      break
    }
    case types.RESTART_BROWSER: {
      chrome.send('brave_rewards.restartBrowser')
      break
    }
    case types.ON_PREF_CHANGED: {
      chrome.send('brave_rewards.getEnabledInlineTippingPlatforms')
      chrome.send('brave_rewards.getContributionAmount')
      chrome.send('brave_rewards.getAutoContributeProperties')
      chrome.send('brave_rewards.getAdsData')
      break
    }
  }

  return state
}

export default rewardsReducer
