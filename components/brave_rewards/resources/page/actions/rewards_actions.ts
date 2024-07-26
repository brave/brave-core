/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'
import { types } from './rewards_types'
import * as mojom from '../../shared/lib/mojom'
import * as Rewards from '../lib/types'

export const isInitialized = () => action(types.IS_INITIALIZED)

export const onSettingSave = (key: string, value: any, persist: boolean = true) => action(types.ON_SETTING_SAVE, {
  key,
  value,
  persist
})

export const onUserType =
  (userType: number) => action(types.ON_USER_TYPE, { userType })

export const getUserType = () => action(types.GET_USER_TYPE)

export const isTermsOfServiceUpdateRequired =
  () => action(types.IS_TERMS_OF_SERVICE_UPDATE_REQUIRED)

export const onIsTermsOfServiceUpdateRequired = (updateRequired: boolean) =>
  action(types.ON_IS_TERMS_OF_SERVICE_UPDATE_REQUIRED, { updateRequired })

export const acceptTermsOfServiceUpdate =
  () => action(types.ACCEPT_TERMS_OF_SERVICE_UPDATE)

export const getRewardsParameters = () => action(types.GET_REWARDS_PARAMETERS)

export const onRewardsParameters = (properties: Rewards.RewardsParameters) =>
  action(types.ON_REWARDS_PARAMETERS, {
    properties
  })

export const getIsAutoContributeSupported = () => action(types.GET_IS_AUTO_CONTRIBUTE_SUPPORTED)

export const onIsAutoContributeSupported = (isAcSupported: boolean) => action(types.ON_IS_AUTO_CONTRIBUTE_SUPPORTED, {
  isAcSupported
})

export const getAutoContributeProperties = () => action(types.GET_AUTO_CONTRIBUTE_PROPERTIES)

export const onAutoContributeProperties = (properties: any) => action(types.ON_AUTO_CONTRIBUTE_PROPERTIES, {
  properties
})

export const onModalResetClose = () => action(types.ON_MODAL_RESET_CLOSE)

export const onModalResetOpen = () => action(types.ON_MODAL_RESET_OPEN)

export const onModalConnectClose = () => action(types.ON_MODAL_CONNECT_CLOSE)

export const onModalConnectOpen = () => action(types.ON_MODAL_CONNECT_OPEN)

export const onModalAdsHistoryClose = () => action(types.ON_MODAL_ADS_HISTORY_CLOSE)

export const onModalAdsHistoryOpen = () => action(types.ON_MODAL_ADS_HISTORY_OPEN)

export const onAdsSettingsClose = () => action(types.ON_ADS_SETTINGS_CLOSE)

export const onAdsSettingsOpen = () => action(types.ON_ADS_SETTINGS_OPEN)

export const onAutoContributeSettingsClose = () => action(types.ON_AUTO_CONTRIBUTE_SETTINGS_CLOSE)

export const onAutoContributeSettingsOpen = () => action(types.ON_AUTO_CONTRIBUTE_SETTINGS_OPEN)

export const onClearAlert = (property: string) => action(types.ON_CLEAR_ALERT, {
  property
})

export const onReconcileStamp = (stamp: number) => action(types.ON_RECONCILE_STAMP, {
  stamp
})

export const onContributeList = (list: Rewards.Publisher[]) => action(types.ON_CONTRIBUTE_LIST, {
  list
})

export const onExcludedList = (list: Rewards.ExcludedPublisher[]) => action(types.ON_EXCLUDED_LIST, {
  list
})

export const onBalanceReport = (properties: { month: number, year: number, report: Rewards.BalanceReport }) => action(types.ON_BALANCE_REPORT, {
  month: properties.month,
  year: properties.year,
  report: properties.report
})

export const onExternalWalletProviderList = (list: Rewards.ExternalWalletProvider[]) => action(types.ON_EXTERNAL_WALLET_PROVIDER_LIST, {
  list
})

export const beginExternalWalletLogin = (provider: string) => action(types.BEGIN_EXTERNAL_WALLET_LOGIN, {
  provider
})

export const onExternalWalletLoginError = () => action(types.ON_EXTERNAL_WALLET_LOGIN_ERROR)

export const getBalanceReport = (month: number, year: number) => action(types.GET_BALANCE_REPORT, {
  month,
  year
})

export const excludePublisher = (publisherKey: string) => action(types.ON_EXCLUDE_PUBLISHER, {
  publisherKey
})

export const restorePublishers = () => action(types.ON_RESTORE_PUBLISHERS)

export const getContributionAmount = () => action(types.GET_CONTRIBUTION_AMOUNT)

export const onContributionAmount = (amount: number) => action(types.ON_CONTRIBUTION_AMOUNT, {
  amount
})

export const onRecurringTips = (list: Rewards.Publisher[]) => action(types.ON_RECURRING_TIPS, {
  list
})

export const removeRecurringTip = (publisherKey: string) => action(types.REMOVE_RECURRING_TIP, {
  publisherKey
})

export const onCurrentTips = (list: Rewards.Publisher[]) => action(types.ON_CURRENT_TIPS, {
  list
})

export const getTipTable = () => action(types.GET_TIP_TABLE)

export const getContributeList = () => action(types.GET_CONTRIBUTE_LIST)

export const getAdsData = () => action(types.GET_ADS_DATA)

export const onAdsData = (adsData: Rewards.AdsData) => action(types.ON_ADS_DATA, {
  adsData
})

export const getAdsHistory = () => action(types.GET_ADS_HISTORY)

export const onAdsHistory = (adsHistory: Rewards.AdsHistory[]) => action(types.ON_ADS_HISTORY, {
  adsHistory
})

export const toggleAdThumbUp = (adHistory: Rewards.AdHistory) => action(types.TOGGLE_AD_THUMB_UP, {
  adHistory
})

export const onToggleAdThumbUp = (success: boolean) => action(types.ON_TOGGLE_AD_THUMB_UP, {
  success
})

export const toggleAdThumbDown = (adHistory: Rewards.AdHistory) => action(types.TOGGLE_AD_THUMB_DOWN, {
  adHistory
})

export const onToggleAdThumbDown = (success: boolean) => action(types.ON_TOGGLE_AD_THUMB_DOWN, {
  success
})

export const toggleAdOptIn = (adHistory: Rewards.AdHistory) => action(types.TOGGLE_AD_OPT_IN, {
  adHistory
})

export const onToggleAdOptIn = (success: boolean) => action(types.ON_TOGGLE_AD_OPT_IN, {
  success
})

export const toggleAdOptOut = (adHistory: Rewards.AdHistory) => action(types.TOGGLE_AD_OPT_OUT, {
  adHistory
})

export const onToggleAdOptOut = (success: boolean) => action(types.ON_TOGGLE_AD_OPT_OUT, {
  success
})

export const toggleSavedAd = (adHistory: Rewards.AdHistory) => action(types.TOGGLE_SAVED_AD, {
  adHistory
})

export const onToggleSavedAd = (success: boolean) => action(types.ON_TOGGLE_SAVED_AD, {
  success
})

export const toggleFlaggedAd = (adHistory: Rewards.AdHistory) => action(types.TOGGLE_FLAGGED_AD, {
  adHistory
})

export const onToggleFlaggedAd = (success: boolean) => action(types.ON_TOGGLE_FLAGGED_AD, {
  success
})

export const onAdsSettingSave = (key: string, value: any) => action(types.ON_ADS_SETTING_SAVE, {
  key,
  value
})

export const getReconcileStamp = () => action(types.GET_RECONCILE_STAMP)

export const onStatement = (data: any) => action(types.ON_STATEMENT, { data })

export const getStatement = () => action(types.GET_STATEMENT)

export const onStatementChanged = () => action(types.ON_STATEMENT_CHANGED)

export const onRecurringTipSaved = (success: boolean) => action(types.ON_RECURRING_TIP_SAVED, {
  success
})

export const onRecurringTipRemoved = (success: boolean) => action(types.ON_RECURRING_TIP_REMOVED, {
  success
})

export const restorePublisher = (publisherKey: string) => action(types.ON_RESTORE_PUBLISHER, {
  publisherKey
})

export const getExcludedSites = () => action(types.GET_EXCLUDED_SITES)

export const getBalance = () => action(types.GET_BALANCE)

export const onBalance = (balance?: mojom.Balance) =>
  action(types.ON_BALANCE, { balance })

export const getExternalWalletProviders = () => action(types.GET_EXTERNAL_WALLET_PROVIDERS)

export const getExternalWallet = () => action(types.GET_EXTERNAL_WALLET)

export const onGetExternalWallet = (externalWallet?: mojom.ExternalWallet) =>
  action(types.ON_GET_EXTERNAL_WALLET, { externalWallet })

export const connectExternalWallet = (path: string, query: string) => action(types.CONNECT_EXTERNAL_WALLET, {
  path,
  query
})

export const onConnectExternalWallet = (result: mojom.ConnectExternalWalletResult) => action(types.ON_CONNECT_EXTERNAL_WALLET, {
  result
})

export const hideRedirectModal = () => action(types.HIDE_REDIRECT_MODAL)

export const onReconcileStampReset = () => action(types.ON_RECONCILE_STAMP_RESET)

export const dismissPromoPrompt = (promo: string) => action(types.DISMISS_PROMO_PROMPT, {
  promo
})

export const getCountryCode = () => action(types.GET_COUNTRY_CODE)

export const onCountryCode = (countryCode: string) => action(types.ON_COUNTRY_CODE, {
  countryCode
})

export const onInitialized = () => action(types.ON_INITIALIZED)

export const completeReset = () => action(types.COMPLETE_RESET)

export const onCompleteReset = (success: boolean) => action(types.ON_COMPLETE_RESET, {
  success
})

export const getOnboardingStatus = () => action(types.GET_ONBOARDING_STATUS)

export const onOnboardingStatus = (showOnboarding: boolean) => action(types.ON_ONBOARDING_STATUS, {
  showOnboarding
})

export const enableRewards = () => action(types.ENABLE_REWARDS)

export const restartBrowser = () => action(types.RESTART_BROWSER)

export const onPrefChanged = (key: string) => action(types.ON_PREF_CHANGED, {
  key
})

export const getIsUnsupportedRegion = () => action(types.GET_IS_UNSUPPORTED_REGION)

export const onIsUnsupportedRegion = (isUnsupportedRegion: boolean) => action(types.ON_IS_UNSUPPORTED_REGION, {
  isUnsupportedRegion
})
