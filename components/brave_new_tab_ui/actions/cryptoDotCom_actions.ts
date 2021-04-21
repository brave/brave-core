// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from '../constants/cryptoDotCom_types'

export const onBtcPriceOptIn = () => action(types.ON_BTC_PRICE_OPT_IN)

export const onCryptoDotComAssetsDetailsReceived = (charts: object, tickerPrices: object, depositAddress: object) => {
  return action(types.ALL_ASSETS_DETAILS_RECEIVED, { charts, tickerPrices, depositAddress })
}

export const onIsConnectedReceived = (isConnected: boolean) => {
  return action(types.ON_IS_CONNECTED_RECEIVED, { isConnected })
}

export const onCryptoDotComRefreshedDataReceived = (tickerPrices: object, losersGainers: object, charts?: object, accountBalances?: object, newsEvents?: object, pairs?: object) => {
  return action(types.REFRESHED_DATA_RECEIVED, { tickerPrices, losersGainers, charts, accountBalances, newsEvents, pairs })
}

export const setCryptoDotComDisconnectInProgress = (inProgress: boolean) => action(types.SET_DISCONNECT_IN_PROGRESS, {
  inProgress
})

export const setCryptoDotComHideBalance = (hide: boolean) => {
  return action(types.HIDE_BALANCE, { hide })
}
