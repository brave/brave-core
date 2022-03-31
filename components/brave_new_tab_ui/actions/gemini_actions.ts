// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from '../constants/gemini_types'

export const onGeminiClientUrl = (clientUrl: string) => action(types.ON_GEMINI_CLIENT_URL, {
  clientUrl
})

export const onValidGeminiAuthCode = () => action(types.ON_VALID_GEMINI_AUTH_CODE)

export const connectToGemini = () => action(types.CONNECT_TO_GEMINI)

export const disconnectGemini = () => action(types.DISCONNECT_GEMINI)

export const setGeminiDisconnectInProgress = (inProgress: boolean) => action(types.SET_DISCONNECT_IN_PROGRESS, {
  inProgress
})

export const setGeminiTickerPrice = (asset: string, price: string) => action(types.SET_GEMINI_TICKER_PRICE, {
  asset,
  price
})

export const setGeminiSelectedView = (view: string) => action(types.SET_SELECTED_VIEW, {
  view
})

export const setGeminiHideBalance = (hide: boolean) => action(types.SET_HIDE_BALANCE, {
  hide
})

export const setGeminiAccountBalances = (balances: Record<string, string>) => action(types.SET_ACCOUNT_BALANCES, {
  balances
})

export type GeminiAssetAddress = {
  asset: string
  address: string
  qrCode: string
}

export const setGeminiAssetAddress = (assetAddresses: GeminiAssetAddress[]) => action(types.SET_ASSET_ADDRESS, {
  assetAddresses
})

export const setGeminiAuthInvalid = (authInvalid: boolean) => action(types.SET_AUTH_INVALID, {
  authInvalid
})
