// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from '../constants/cryptoDotCom_types'

export const onBtcPriceOptIn = () => action(types.ON_BTC_PRICE_OPT_IN)

export const onCryptoDotComMarketsRequested = (markets: string[]) => {
  return action(types.MARKETS_RECEIVED, { markets })
}

export const onCryptoDotComMarketDataReceived = (tickerPrices: object, losersGainers: object[]) => {
  return action(types.MARKETS_RECEIVED, { tickerPrices, losersGainers })
}

export const onCryptoDotComAssetsDetailsRequested = (asset: string) => {
  return action(types.ALL_ASSETS_DETAILS_REQUESTED, asset)
}

export const onCryptoDotComAssetsDetailsReceived = (charts: object, pairs: object[]) => {
  return action(types.ALL_ASSETS_DETAILS_RECEIVED, { charts, pairs })
}

export const onCryptoDotComRefreshData = () => {
  return action(types.ON_REFRESH_DATA)
}

export const onCryptoDotComRefreshedDataReceived = (tickerPrices: object, losersGainers: object[], charts: object) => {
  return action(types.REFRESHED_DATA_RECEIVED, { tickerPrices, losersGainers, charts })
}

export const onCryptoDotComBuyCrypto = () => action(types.ON_BUY_CRYPTO)

export const onCryptoDotComOptInMarkets = (show: boolean) => {
  return action(types.ON_MARKETS_OPT_IN, { show })
}
