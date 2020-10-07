/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constant
import { types } from '../constants/rewards_types'

export const isInitialized = () => action(types.IS_INITIALIZED)

export const createWallet = () => action(types.CREATE_WALLET)

export const onWalletCreated = () => action(types.WALLET_CREATED)

export const onWalletCreateFailed = () => action(types.WALLET_CREATE_FAILED)

export const onSettingSave = (key: string, value: any) => action(types.ON_SETTING_SAVE, {
  key,
  value
})

export const updateAdsRewards = () => action(types.UPDATE_ADS_REWARDS)

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

export const onPromotionFinish = (properties: Rewards.PromotionFinish) => action(types.ON_PROMOTION_FINISH, {
  properties
})

export const deletePromotion = (promotionId: string) => action(types.DELETE_PROMOTION, {
  promotionId
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

export const checkWalletExistence = () => action(types.CHECK_WALLET_EXISTENCE)

export const onWalletExists = (exists: boolean) => action(types.ON_WALLET_EXISTS, {
  exists
})

export const restorePublishers = () => action(types.ON_RESTORE_PUBLISHERS)

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

export const onRewardsEnabled = (enabled: boolean) => action(types.ON_REWARDS_ENABLED, {
  enabled
})

export const onTransactionHistory = (data: {adsEstimatedPendingRewards: number, adsNextPaymentDate: string, adsNotificationsReceivedThisMonth: number}) =>
  action(types.ON_TRANSACTION_HISTORY, {
    data
  })

export const getTransactionHistory = () => action(types.GET_TRANSACTION_HISTORY)

export const onTransactionHistoryChanged = () => action(types.ON_TRANSACTION_HISTORY_CHANGED)

export const getRewardsMainEnabled = () => action(types.GET_REWARDS_MAIN_ENABLED)

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

export const onlyAnonWallet = () => action(types.ONLY_ANON_WALLET)

export const onOnlyAnonWallet = (only: boolean) => action(types.ON_ONLY_ANON_WALLET, {
  only
})

export const toggleEnableMain = (enable: boolean) => action(types.TOGGLE_ENABLE_MAIN, {
  enable
})

export const onInitialized = (result: boolean) => action(types.ON_INITIALIZED, {
  result
})
