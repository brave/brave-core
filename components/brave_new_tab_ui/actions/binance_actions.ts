// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from '../constants/binance_types'

export const onBinanceUserTLD = (userTLD: NewTab.BinanceTLD) => action(types.ON_BINANCE_USER_TLD, {
  userTLD
})

export const onBinanceUserLocale = (userLocale: string) => action(types.ON_BINANCE_USER_LOCALE, {
  userLocale
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

export const setBinanceSupported = (supported: boolean) => action(types.SET_BINANCE_SUPPORTED, {
  supported
})

export const onBinanceClientUrl = (clientUrl: string) => action(types.ON_BINANCE_CLIENT_URL, {
  clientUrl
})

export const onValidBinanceAuthCode = () => action(types.ON_VALID_BINANCE_AUTH_CODE)

export const setBinanceHideBalance = (hide: boolean) => action(types.SET_HIDE_BALANCE, {
  hide
})

export const connectToBinance = () => action(types.CONNECT_TO_BINANCE)

export const disconnectBinance = () => action(types.DISCONNECT_BINANCE)

export const onAssetsBalanceInfo = (info: Record<string, Record<string, string>>) => action(types.ON_ASSETS_BALANCE_INFO, {
  info
})

export const onAssetDepositInfo = (symbol: string, address: string, tag: string) => action(types.ON_ASSET_DEPOSIT_INFO, {
  symbol,
  address,
  tag
})

export const onDepositQRForAsset = (asset: string, imageSrc: string) => action(types.ON_DEPOSIT_QR_FOR_ASSET, {
  asset,
  imageSrc
})

export const onConvertableAssets = (assets: chrome.binance.ConvertAssets) => action(types.ON_CONVERTABLE_ASSETS, {
  assets
})

export const setBinanceDisconnectInProgress = (inProgress: boolean) => action(types.SET_DISCONNECT_IN_PROGRESS, {
  inProgress
})

export const setAuthInvalid = (authInvalid: boolean) => action(types.SET_AUTH_INVALID, {
  authInvalid
})

export const setBinanceSelectedView = (view: string) => action(types.SET_SELECTED_VIEW, {
  view
})

export const setDepositInfoSaved = () => action(types.SET_DEPOSIT_INFO_SAVED)
