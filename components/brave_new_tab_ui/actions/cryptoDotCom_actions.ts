// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from '../constants/cryptoDotCom_types'

export const onBtcPriceOptIn = () => action(types.ON_BTC_PRICE_OPT_IN)

export const cryptoDotComMarketDataUpdate = (tickerPrices: object, losersGainers: object[]) => {
  return action(types.MARKET_DATA_UPDATED, { tickerPrices, losersGainers })
}

export const setCryptoDotComAssetData = (charts: object, pairs: object[]) => {
  return action(types.SET_ASSET_DATA, { charts, pairs })
}

export const onCryptoDotComRefreshData = (tickerPrices: object, losersGainers: object[], charts: object) => {
  return action(types.ON_REFRESH_DATA, { tickerPrices, losersGainers, charts })
}

export const setCryptoDotComSupportedPairs = (pairs: object[]) => {
  return action(types.SET_SUPPORTED_PAIRS, { pairs })
}

export const onCryptoDotComBuyCrypto = () => action(types.ON_BUY_CRYPTO)

export const onCryptoDotComInteraction = () => action(types.ON_INTERACTION)

export const onCryptoDotComOptInMarkets = (show: boolean) => {
  return action(types.ON_MARKETS_OPT_IN, { show })
}
