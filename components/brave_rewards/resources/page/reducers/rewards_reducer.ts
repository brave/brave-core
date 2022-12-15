/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

import { types } from '../actions/rewards_types'
import { userTypeFromMojo } from '../../shared/lib/user_type'

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
    case types.ON_MODAL_BACKUP_CLOSE: {
      let ui = state.ui
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
      state.adsData.adsEnabled = action.payload.adsData.adsEnabled
      state.adsData.adsPerHour = action.payload.adsData.adsPerHour
      state.adsData.adsSubdivisionTargeting = action.payload.adsData.adsSubdivisionTargeting
      state.adsData.automaticallyDetectedAdsSubdivisionTargeting = action.payload.adsData.automaticallyDetectedAdsSubdivisionTargeting
      state.adsData.shouldAllowAdsSubdivisionTargeting = action.payload.adsData.shouldAllowAdsSubdivisionTargeting
      state.adsData.subdivisions = action.payload.adsData.subdivisions
      state.adsData.adsUIEnabled = action.payload.adsData.adsUIEnabled
      state.adsData.adsIsSupported = action.payload.adsData.adsIsSupported
      state.adsData.needsBrowserUpgradeToServeAds = action.payload.adsData.needsBrowserUpgradeToServeAds
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
          state.adsData = { ...state.adsData }
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
    case types.CONNECT_EXTERNAL_WALLET: {
      const path = action.payload.path
      const query = action.payload.query
      const ui = state.ui

      chrome.send('brave_rewards.connectExternalWallet', [path, query])
      ui.modalRedirect = 'show'

      state = { ...state, ui }
      break
    }
    case types.ON_CONNECT_EXTERNAL_WALLET: {
      chrome.send('brave_rewards.getExternalWallet')

      const ui = state.ui
      const { value, error } = action.payload.result

      if (value) {
        chrome.send('brave_rewards.getUserType')
        chrome.send('brave_rewards.fetchBalance')
        ui.modalRedirect = 'hide'
      } else {
        ui.modalRedirect = error
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
      chrome.send('brave_rewards.getEnabledInlineTippingPlatforms')
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
