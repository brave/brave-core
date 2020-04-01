// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/new_tab_types'
import { Preferences } from '../api/preferences'
import { Stats } from '../api/stats'
import { PrivateTabData } from '../api/privateTabData'
import { InitialData, InitialRewardsData, PreInitialRewardsData } from '../api/initialData'

export const statsUpdated = (stats: Stats) =>
  action(types.NEW_TAB_STATS_UPDATED, {
    stats
  })

export const privateTabDataUpdated = (data: PrivateTabData) =>
  action(types.NEW_TAB_PRIVATE_TAB_DATA_UPDATED, data)

export const dismissBrandedWallpaperNotification = (isUserAction: boolean) =>
  action(types.NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION, {
    isUserAction
  })

export const preferencesUpdated = (preferences: Preferences) =>
  action(types.NEW_TAB_PREFERENCES_UPDATED, preferences)

export const setInitialData = (initialData: InitialData) =>
  action(types.NEW_TAB_SET_INITIAL_DATA, initialData)

export const createWallet = () => action(types.CREATE_WALLET, {})

export const onEnabledMain = (enabledMain: boolean, enabledAds?: boolean) => action(types.ON_ENABLED_MAIN, {
  enabledMain,
  enabledAds
})

export const onAdsEnabled = (enabled: boolean) => action(types.ON_ADS_ENABLED, {
  enabled
})

export const onWalletInitialized = (result: NewTab.RewardsResult) => action(types.ON_WALLET_INITIALIZED, {
  result
})

export const onAdsEstimatedEarnings = (amount: number) => action(types.ON_ADS_ESTIMATED_EARNINGS, {
  amount
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

export const onWalletExists = (exists: boolean) => action(types.ON_WALLET_EXISTS, {
  exists
})

export const setInitialRewardsData = (initialRewardsData: InitialRewardsData) => action(types.SET_INITIAL_REWARDS_DATA, initialRewardsData)

export const setPreInitialRewardsData = (preInitialRewardsData: PreInitialRewardsData) => action(types.SET_PRE_INITIAL_REWARDS_DATA, preInitialRewardsData)

export const onPromotionFinish = (result: NewTab.RewardsResult, promotion: NewTab.Promotion) => action(types.ON_PROMOTION_FINISH, {
  result,
  promotion
})

export const setCurrentStackWidget = (widgetId: NewTab.StackWidget) => action(types.SET_CURRENT_STACK_WIDGET, {
  widgetId
})

export const onBinanceUserTLD = (userTLD: NewTab.BinanceTLD) => action(types.ON_BINANCE_USER_TLD, {
  userTLD
})

export const setInitialFiat = (initialFiat: string) => action(types.SET_INITIAL_FIAT, {
  initialFiat
})

export const setInitialAmount = (initialAmount: string) => action(types.SET_INITIAL_AMOUNT, {
  initialAmount
})

export const setInitialAsset = (initialAsset: string) => action(types.SET_INITIAL_ASSET, {
  initialAsset
})

export const setUserTLDAutoSet = () => action(types.SET_USER_TLD_AUTO_SET)

export const setOnlyAnonWallet = (onlyAnonWallet: boolean) => action(types.SET_ONLY_ANON_WALLET, {
  onlyAnonWallet
})

export const setBinanceSupported = (supported: boolean) => action(types.SET_BINANCE_SUPPORTED, {
  supported
})

export const onBinanceClientUrl = (clientUrl: string) => action(types.ON_BINANCE_CLIENT_URL, {
  clientUrl
})

export const onValidAuthCode = () => action(types.ON_VALID_AUTH_CODE)

export const setHideBalance = (hide: boolean) => action(types.SET_HIDE_BALANCE, {
  hide
})

export const connectToBinance = () => action(types.CONNECT_TO_BINANCE)

export const disconnectBinance = () => action(types.DISCONNECT_BINANCE)

export const onBinanceAccountBalances = (balances: Record<string, string>) => action(types.ON_BINANCE_ACCOUNT_BALANCES, {
  balances
})
