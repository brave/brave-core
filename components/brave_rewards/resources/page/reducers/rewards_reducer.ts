/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

import { types } from '../actions/rewards_types'
import { userTypeFromMojo } from '../../shared/lib/user_type'
import * as Rewards from '../lib/types'
import * as mojom from '../../shared/lib/mojom'

const rewardsReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  if (!state) {
    return
  }

  switch (action.type) {
    case types.IS_INITIALIZED: {
      chrome.send('brave_rewards.isInitialized')
      break
    }
    case types.GET_USER_TYPE: {
      chrome.send('brave_rewards.getUserType')
      break
    }
    case types.ON_USER_TYPE: {
      state = { ...state }
      state.userType = userTypeFromMojo(action.payload.userType)
      break
    }
    case types.IS_TERMS_OF_SERVICE_UPDATE_REQUIRED: {
      chrome.send('brave_rewards.isTermsOfServiceUpdateRequired')
      break
    }
    case types.ON_IS_TERMS_OF_SERVICE_UPDATE_REQUIRED: {
      state = {
        ...state,
        isUserTermsOfServiceUpdateRequired: action.payload.updateRequired
      }
      break
    }
    case types.ACCEPT_TERMS_OF_SERVICE_UPDATE: {
      chrome.send('brave_rewards.acceptTermsOfServiceUpdate')
      chrome.send('brave_rewards.isTermsOfServiceUpdateRequired')
      break
    }
    case types.GET_IS_AUTO_CONTRIBUTE_SUPPORTED: {
      chrome.send('brave_rewards.isAutoContributeSupported')
      break
    }
    case types.ON_IS_AUTO_CONTRIBUTE_SUPPORTED: {
      state = { ...state }
      state.isAcSupported = action.payload.isAcSupported
      break
    }
    case types.GET_AUTO_CONTRIBUTE_PROPERTIES: {
      chrome.send('brave_rewards.getAutoContributeProperties')
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
    case types.ON_MODAL_RESET_CLOSE: {
      let ui = state.ui
      ui.modalReset = false
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_MODAL_RESET_OPEN: {
      let ui = state.ui
      ui.modalReset = true
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_MODAL_ADS_HISTORY_CLOSE: {
      let ui = state.ui
      ui.modalAdsHistory = false
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_MODAL_ADS_HISTORY_OPEN: {
      let ui = state.ui
      ui.modalAdsHistory = true
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_ADS_SETTINGS_CLOSE: {
      let ui = state.ui
      ui.adsSettings = false
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_ADS_SETTINGS_OPEN: {
      let ui = state.ui
      ui.adsSettings = true
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_AUTO_CONTRIBUTE_SETTINGS_CLOSE: {
      let ui = state.ui
      ui.autoContributeSettings = false
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_AUTO_CONTRIBUTE_SETTINGS_OPEN: {
      let ui = state.ui
      ui.autoContributeSettings = true
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_MODAL_CONNECT_CLOSE: {
      let ui = state.ui
      ui.modalConnect = false
      state = {
        ...state,
        ui
      }
      break
    }
    case types.ON_MODAL_CONNECT_OPEN: {
      let ui = state.ui
      ui.modalConnect = true
      ui.modalConnectState = ''
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

      state = {...state}

      const { adsData } = action.payload
      state.adsData.adsPerHour = adsData.adsPerHour
      state.adsData.adsSubdivisionTargeting = adsData.adsSubdivisionTargeting
      state.adsData.automaticallyDetectedAdsSubdivisionTargeting = adsData.automaticallyDetectedAdsSubdivisionTargeting
      state.adsData.shouldAllowAdsSubdivisionTargeting = adsData.shouldAllowAdsSubdivisionTargeting
      state.adsData.subdivisions = adsData.subdivisions
      state.adsData.adsIsSupported = adsData.adsIsSupported
      state.adsData.needsBrowserUpgradeToServeAds = adsData.needsBrowserUpgradeToServeAds
      state.adsData.notificationAdsEnabled = adsData.notificationAdsEnabled
      state.adsData.newTabAdsEnabled = adsData.newTabAdsEnabled
      state.adsData.newsAdsEnabled = adsData.newsAdsEnabled
      state.adsData.searchAdsEnabled = adsData.searchAdsEnabled
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
      state.adsHistory = action.payload.adsHistory
      break
    }
    case types.TOGGLE_AD_THUMB_UP: {
      chrome.send('brave_rewards.toggleAdThumbUp', [action.payload.adHistory])
      break
    }
    case types.ON_TOGGLE_AD_THUMB_UP: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_AD_THUMB_DOWN: {
      chrome.send('brave_rewards.toggleAdThumbDown', [action.payload.adHistory])
      break
    }
    case types.ON_TOGGLE_AD_THUMB_DOWN: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_AD_OPT_IN: {
      chrome.send('brave_rewards.toggleAdOptIn', [action.payload.adHistory])
      break
    }
    case types.ON_TOGGLE_AD_OPT_IN: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_AD_OPT_OUT: {
      chrome.send('brave_rewards.toggleAdOptOut', [action.payload.adHistory])
      break
    }
    case types.ON_TOGGLE_AD_OPT_OUT: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_SAVED_AD: {
      chrome.send('brave_rewards.toggleSavedAd', [action.payload.adHistory])
      break
    }
    case types.ON_TOGGLE_SAVED_AD: {
      chrome.send('brave_rewards.getAdsHistory')
      break
    }
    case types.TOGGLE_FLAGGED_AD: {
      chrome.send('brave_rewards.toggleFlaggedAd', [action.payload.adHistory])
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
        state.adsData = { ...state.adsData }
        state.adsData[key] = value
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

      const data = action.payload.data
      state.adsData.adsNextPaymentDate = data.adsNextPaymentDate
      state.adsData.adsReceivedThisMonth = data.adsReceivedThisMonth
      state.adsData.adTypesReceivedThisMonth = data.adTypesReceivedThisMonth
      state.adsData.adsMinEarningsThisMonth = data.adsMinEarningsThisMonth
      state.adsData.adsMaxEarningsThisMonth = data.adsMaxEarningsThisMonth
      state.adsData.adsMinEarningsLastMonth = data.adsMinEarningsLastMonth
      state.adsData.adsMaxEarningsLastMonth = data.adsMaxEarningsLastMonth
      break
    }
    case types.CONNECT_EXTERNAL_WALLET: {
      const path = action.payload.path
      const query = action.payload.query
      const ui = state.ui

      chrome.send('brave_rewards.connectExternalWallet', [path, query])
      ui.modalRedirect = 'show'
      // The first non-empty path segment contains the wallet provider type.
      ui.modalRedirectProvider = path.replace(/^\//, '').split('/')[0] || ''

      state = { ...state, ui }
      break
    }
    case types.ON_CONNECT_EXTERNAL_WALLET: {
      chrome.send('brave_rewards.getExternalWallet')

      const ui = state.ui
      const { result } = action.payload

      if (result === mojom.ConnectExternalWalletResult.kSuccess) {
        chrome.send('brave_rewards.getUserType')
        chrome.send('brave_rewards.fetchBalance')
        ui.modalRedirect = 'hide'
      } else {
        ui.modalRedirect = result
      }

      state = { ...state, ui }

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
    case types.DISMISS_PROMO_PROMPT: {
      const ui = state.ui
      const promoKey = action.payload.promo

      ui.promosDismissed[promoKey] = true

      state = {
        ...state,
        ui
      }

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
      chrome.send('brave_rewards.getReconcileStamp')
      state = {
        ...state,
        initializing: false
      }
      break
    }
    case types.COMPLETE_RESET: {
      chrome.send('brave_rewards.completeReset')
      break
    }
    case types.ON_COMPLETE_RESET: {
      if (action.payload.success) {
        chrome.send('brave_rewards.getOnboardingStatus')
        return undefined
      }
      break
    }
    case types.GET_ONBOARDING_STATUS: {
      chrome.send('brave_rewards.getOnboardingStatus')
      break
    }
    case types.ON_ONBOARDING_STATUS: {
      let { showOnboarding } = action.payload
      state = {
        ...state,
        showOnboarding
      }
      break
    }
    case types.ENABLE_REWARDS: {
      chrome.send('brave_rewards.enableRewards')
      break
    }
    case types.RESTART_BROWSER: {
      chrome.send('brave_rewards.restartBrowser')
      break
    }
    case types.ON_PREF_CHANGED: {
      chrome.send('brave_rewards.getContributionAmount')
      chrome.send('brave_rewards.getAutoContributeProperties')
      chrome.send('brave_rewards.getAdsData')
      break
    }
    case types.GET_IS_UNSUPPORTED_REGION: {
      chrome.send('brave_rewards.getIsUnsupportedRegion')
      break
    }
    case types.ON_IS_UNSUPPORTED_REGION: {
      const { isUnsupportedRegion } = action.payload
      state = {
        ...state,
        isUnsupportedRegion
      }
      break
    }
  }

  return state
}

export default rewardsReducer
