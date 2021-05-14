/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constant
import { types } from '../constants/rewards_types'

export const isInitialized = () => action(types.IS_INITIALIZED)

export const onSettingSave = (key: string, value: any, persist: boolean = true) => action(types.ON_SETTING_SAVE, {
  key,
  value,
  persist
})

export const getRewardsParameters = () => action(types.GET_REWARDS_PARAMETERS)

export const onRewardsParameters = (properties: Rewards.RewardsParameters) =>
  action(types.ON_REWARDS_PARAMETERS, {
    properties
  })

export const getAutoContributeProperties = () => action(types.GET_AUTO_CONTRIBUTE_PROPERTIES)

export const onAutoContributeProperties = (properties: any) => action(types.ON_AUTO_CONTRIBUTE_PROPERTIES, {
  properties
})

export const fetchPromotions = () => action(types.FETCH_PROMOTIONS)

export const onPromotions = (properties: Rewards.PromotionResponse) => action(types.ON_PROMOTIONS, {
  properties
})

export const claimPromotion = (promotionId: string) => action(types.CLAIM_PROMOTION, {
  promotionId
})

export const onClaimPromotion = (properties: Rewards.Captcha) => action(types.ON_CLAIM_PROMOTION, {
  properties
})

export const attestPromotion = (promotionId: string, x: number, y: number) => action(types.ATTEST_PROMOTION, {
  promotionId,
  x,
  y
})

export const onPromotionFinish = (properties: Rewards.PromotionFinish) => action(types.ON_PROMOTION_FINISH, {
  properties
})

export const resetPromotion = (promotionId: string) => action(types.RESET_PROMOTION, {
  promotionId
})

export const deletePromotion = (promotionId: string) => action(types.DELETE_PROMOTION, {
  promotionId
})

export const recoverWallet = (key: string) => action(types.RECOVER_WALLET, {
  key
})

export const onRecoverWalletData = (result: number) => action(types.ON_RECOVER_WALLET_DATA, {
  result
})

export const onModalBackupClose = () => action(types.ON_MODAL_BACKUP_CLOSE)

export const onModalBackupOpen = () => action(types.ON_MODAL_BACKUP_OPEN)

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

export const onBalanceReport = (properties: {month: number, year: number, report: Rewards.BalanceReport}) => action(types.ON_BALANCE_REPORT, {
  month: properties.month,
  year: properties.year,
  report: properties.report
})

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

export const toggleAdThumbUp = (uuid: string, creativeSetId: string, likeAction: number) => action(types.TOGGLE_AD_THUMB_UP, {
  uuid,
  creativeSetId,
  likeAction
})

export const onToggleAdThumbUp = (result: Rewards.ToggleLikeAction) => action(types.ON_TOGGLE_AD_THUMB_UP, {
  result
})

export const toggleAdThumbDown = (uuid: string, creativeSetId: string, likeAction: number) => action(types.TOGGLE_AD_THUMB_DOWN, {
  uuid,
  creativeSetId,
  likeAction
})

export const onToggleAdThumbDown = (result: Rewards.ToggleLikeAction) => action(types.ON_TOGGLE_AD_THUMB_DOWN, {
  result
})

export const toggleAdOptInAction = (category: string, optAction: number) => action(types.TOGGLE_AD_OPT_IN_ACTION, {
  category,
  optAction
})

export const onToggleAdOptInAction = (result: Rewards.ToggleOptAction) => action(types.ON_TOGGLE_AD_OPT_IN_ACTION, {
  result
})

export const toggleAdOptOutAction = (category: string, optAction: number) => action(types.TOGGLE_AD_OPT_OUT_ACTION, {
  category,
  optAction
})

export const onToggleAdOptOutAction = (result: Rewards.ToggleOptAction) => action(types.ON_TOGGLE_AD_OPT_OUT_ACTION, {
  result
})

export const toggleSaveAd = (uuid: string, creativeSetId: string, savedAd: boolean) => action(types.TOGGLE_SAVE_AD, {
  uuid,
  creativeSetId,
  savedAd
})

export const onToggleSaveAd = (result: Rewards.ToggleSaveAd) => action(types.ON_TOGGLE_SAVE_AD, {
  result
})

export const toggleFlagAd = (uuid: string, creativeSetId: string, flaggedAd: boolean) => action(types.TOGGLE_FLAG_AD, {
  uuid,
  creativeSetId,
  flaggedAd
})

export const onToggleFlagAd = (result: Rewards.ToggleFlagAd) => action(types.ON_TOGGLE_FLAG_AD, {
  result
})

export const onAdsSettingSave = (key: string, value: any) => action(types.ON_ADS_SETTING_SAVE, {
  key,
  value
})

export const getReconcileStamp = () => action(types.GET_RECONCILE_STAMP)

export const getPendingContributions = () => action(types.GET_PENDING_CONTRIBUTIONS)

export const onPendingContributions = (list: Rewards.PendingContribution[]) =>
  action(types.ON_PENDING_CONTRIBUTIONS, {
    list
  })

export const onStatement = (data: any) => action(types.ON_STATEMENT, { data })

export const getStatement = () => action(types.GET_STATEMENT)

export const onStatementChanged = () => action(types.ON_STATEMENT_CHANGED)

export const onRecurringTipSaved = (success: boolean) => action(types.ON_RECURRING_TIP_SAVED, {
  success
})

export const onRecurringTipRemoved = (success: boolean) => action(types.ON_RECURRING_TIP_REMOVED, {
  success
})

export const onInlineTipSettingChange = (key: string, value: boolean) => action(types.ON_INLINE_TIP_SETTINGS_CHANGE, {
  key,
  value
})

export const removePendingContribution = (id: number) =>
  action(types.REMOVE_PENDING_CONTRIBUTION, {
    id
  })

export const removeAllPendingContribution = () => action(types.REMOVE_ALL_PENDING_CONTRIBUTION)

export const restorePublisher = (publisherKey: string) => action(types.ON_RESTORE_PUBLISHER, {
  publisherKey
})

export const getExcludedSites = () => action(types.GET_EXCLUDED_SITES)

export const getBalance = () => action(types.GET_BALANCE)

export const onBalance = (status: number, balance: Rewards.Balance) => action(types.ON_BALANCE, {
  status,
  balance
})

export const getExternalWallet = () => action(types.GET_EXTERNAL_WALLET)

export const onExternalWallet = (result: number, wallet: Rewards.ExternalWallet) => action(types.ON_EXTERNAL_WALLET, {
  result,
  wallet
})

export const onVerifyOnboardingDisplayed = () => action(types.ON_VERIFY_ONBOARDING_DISPLAYED)

export const processRewardsPageUrl = (path: string, query: string) => action(types.PROCESS_REWARDS_PAGE_URL, {
  path,
  query
})

export const onProcessRewardsPageUrl = (data: Rewards.ProcessRewardsPageUrl) => action(types.ON_PROCESS_REWARDS_PAGE_URL, {
  data
})

export const hideRedirectModal = () => action(types.HIDE_REDIRECT_MODAL)

export const disconnectWallet = () => action(types.DISCONNECT_WALLET)

export const getMonthlyReport = (month?: number, year?: number) => action(types.GET_MONTHLY_REPORT, {
  month,
  year
})

export const onMonthlyReport = (properties: { result: number, month: number, year: number, report: Rewards.MonthlyReport}) => action(types.ON_MONTHLY_REPORT, {
  result: properties.result,
  month: properties.month,
  year: properties.year,
  report: properties.report
})

export const onReconcileStampReset = () => action(types.ON_RECONCILE_STAMP_RESET)

export const getMonthlyReportIds = () => action(types.GET_MONTHLY_REPORT_IDS)

export const onMonthlyReportIds = (ids: string[]) => action(types.ON_MONTHLY_REPORT_IDS, ids)

export const dismissPromoPrompt = (promo: string) => action(types.DISMISS_PROMO_PROMPT, {
  promo
})

export const getCountryCode = () => action(types.GET_COUNTRY_CODE)

export const onCountryCode = (countryCode: string) => action(types.ON_COUNTRY_CODE, {
  countryCode
})

export const onInitialized = (result: boolean) => action(types.ON_INITIALIZED, {
  result
})

export const completeReset = () => action(types.COMPLETE_RESET)

export const onCompleteReset = (success: boolean) => action(types.ON_COMPLETE_RESET, {
  success
})

export const getPaymentId = () => action(types.GET_PAYMENT_ID)

export const disconnectWalletError = () => action(types.DISCONNECT_WALLET_ERROR)

export const onPaymentId = (paymentId: string) => action(types.ON_PAYMENT_ID, {
  paymentId
})

export const setFirstLoad = (firstLoad: boolean) => action(types.SET_FIRST_LOAD, {
  firstLoad
})

export const getWalletPassphrase = () => action(types.GET_WALLET_PASSPHRASE)

export const onWalletPassphrase = (passphrase: string) => action(types.ON_WALLET_PASSPHRASE, {
  passphrase
})

export const getOnboardingStatus = () => action(types.GET_ONBOARDING_STATUS)

export const onOnboardingStatus = (showOnboarding: boolean) => action(types.ON_ONBOARDING_STATUS, {
  showOnboarding
})

export const saveOnboardingResult = (result: 'opted-in' | 'dismissed') => action(types.SAVE_ONBOARDING_RESULT, {
  result
})
