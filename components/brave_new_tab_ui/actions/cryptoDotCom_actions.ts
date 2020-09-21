// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from '../constants/cryptoDotCom_types'

export const onTotalPriceOptIn = () => action(types.ON_TOTAL_PRICE_OPT_IN)

export const onBtcPriceOptIn = () => action(types.ON_BTC_PRICE_OPT_IN)

export const setCryptoDotComTickerPrices = (assetData: object) => action(types.SET_TICKER_PRICES, assetData)

export const setCryptoDotComLosersGainers = (losersGainers: object) => action(types.SET_LOSERS_GAINERS, losersGainers)

export const setCryptoDotComChartData = (asset: string, chartData: Array<object>) => action(types.SET_CHART_DATA, { asset, chartData })
