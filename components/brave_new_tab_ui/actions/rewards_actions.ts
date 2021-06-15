// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from '../constants/rewards_types'
import { InitialRewardsData, PreInitialRewardsData } from '../api/initialData'

export const onAdsEnabled = (enabled: boolean) => action(types.ON_ADS_ENABLED, {
  enabled
})

export const onBalanceReport = (properties: {month: number, year: number, report: NewTab.RewardsBalanceReport}) => action(types.ON_BALANCE_REPORT, {
  month: properties.month,
  year: properties.year,
  report: properties.report
})

export const onPromotions = (result: number, promotions: NewTab.Promotion[]) => action(types.ON_PROMOTIONS, {
  result,
  promotions
})

export const dismissNotification = (id: string) => action(types.DISMISS_NOTIFICATION, {
  id
})

export const onBalance = (balance: NewTab.RewardsBalance) => action(types.ON_BALANCE, {
  balance
})

export const onAdsAccountStatement = (adsAccountStatement: NewTab.AdsAccountStatement) => action(types.ON_ADS_ACCOUNT_STATEMENT, {
  adsAccountStatement
})

export const setInitialRewardsData = (initialRewardsData: InitialRewardsData) => action(types.SET_INITIAL_REWARDS_DATA, initialRewardsData)

export const setPreInitialRewardsData = (preInitialRewardsData: PreInitialRewardsData) => action(types.SET_PRE_INITIAL_REWARDS_DATA, preInitialRewardsData)

export const onPromotionFinish = (result: NewTab.RewardsResult, promotion: NewTab.Promotion) => action(types.ON_PROMOTION_FINISH, {
  result,
  promotion
})

export const onCompleteReset = (success: boolean) => action(types.ON_COMPLETE_RESET, {
  success
})
