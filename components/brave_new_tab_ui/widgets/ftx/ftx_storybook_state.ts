// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { select, boolean } from '@storybook/addon-knobs'
import { FTXState, ConversionData, ViewType } from './ftx_state'

const viewTypeArray = Object.keys(ViewType).filter(v => isNaN(Number(v)))

const balances = {
  BAT: 42.5,
  BTC: 0.00005043,
  USD: 99.54
}

const marketData = [
  {
    percentChangeDay: -0.03,
    price: 1.212075,
    symbol: 'BAT',
    volumeDay: 3554610.609675
  },
  {
    percentChangeDay: -0.03,
    price: 53913,
    symbol: 'BTC',
    volumeDay: 3669478157.1446
  },
  {
    percentChangeDay: -0,
    price: 1.0003,
    symbol: 'USDT',
    volumeDay: 1918011.8944
  }
]

const batMarketData = marketData.find(m => m.symbol === 'BAT')

const chartData = [
  {
    'c': 1.497975,
    'h': 1.50125,
    'l': 1.462675
  },
  {
    'c': 1.513625,
    'h': 1.5283,
    'l': 1.476375
  },
  {
    'c': 1.535575,
    'h': 1.5788,
    'l': 1.49895
  },
  {
    'c': 1.565275,
    'h': 1.6006,
    'l': 1.53435
  },
  {
    'c': 1.526825,
    'h': 1.56675,
    'l': 1.52285
  },
  {
    'c': 1.532025,
    'h': 1.582225,
    'l': 1.5104
  },
  {
    'c': 1.496625,
    'h': 1.532875,
    'l': 1.47415
  },
  {
    'c': 1.4577,
    'h': 1.518125,
    'l': 1.418
  },
  {
    'c': 1.489675,
    'h': 1.4914,
    'l': 1.4057
  },
  {
    'c': 1.5157,
    'h': 1.52795,
    'l': 1.48235
  },
  {
    'c': 1.531225,
    'h': 1.53715,
    'l': 1.494775
  },
  {
    'c': 1.550525,
    'h': 1.55575,
    'l': 1.499975
  },
  {
    'c': 1.547825,
    'h': 1.5609,
    'l': 1.519275
  },
  {
    'c': 1.50635,
    'h': 1.611925,
    'l': 1.490125
  },
  {
    'c': 1.4869,
    'h': 1.556275,
    'l': 1.483
  },
  {
    'c': 1.50395,
    'h': 1.5115,
    'l': 1.461075
  },
  {
    'c': 1.4743,
    'h': 1.540975,
    'l': 1.472975
  },
  {
    'c': 1.2565,
    'h': 1.486875,
    'l': 1.09
  },
  {
    'c': 1.33085,
    'h': 1.353725,
    'l': 1.2165
  },
  {
    'c': 1.1556,
    'h': 1.34005,
    'l': 1.1494
  },
  {
    'c': 1.2788,
    'h': 1.285575,
    'l': 1.154425
  },
  {
    'c': 1.311475,
    'h': 1.324075,
    'l': 1.258375
  },
  {
    'c': 1.34805,
    'h': 1.3505,
    'l': 1.28485
  },
  {
    'c': 1.373475,
    'h': 1.381425,
    'l': 1.296175
  },
  {
    'c': 1.369875,
    'h': 1.3933,
    'l': 1.347325
  },
  {
    'c': 1.31665,
    'h': 1.37505,
    'l': 1.2627
  },
  {
    'c': 1.187925,
    'h': 1.32,
    'l': 1.17795
  },
  {
    'c': 1.25185,
    'h': 1.25585,
    'l': 1.175575
  },
  {
    'c': 1.323675,
    'h': 1.49315,
    'l': 1.243725
  },
  {
    'c': 1.213125,
    'h': 1.336125,
    'l': 1.173125
  },
  {
    'c': 1.197925,
    'h': 1.2543,
    'l': 1.156875
  },
  {
    'c': 1.288775,
    'h': 1.29275,
    'l': 1.1931
  },
  {
    'c': 1.2929,
    'h': 1.3236,
    'l': 1.2461
  },
  {
    'c': 1.286875,
    'h': 1.319725,
    'l': 1.25195
  },
  {
    'c': 1.269225,
    'h': 1.287475,
    'l': 1.231975
  },
  {
    'c': 1.268775,
    'h': 1.3108,
    'l': 1.244425
  },
  {
    'c': 1.2817,
    'h': 1.317175,
    'l': 1.25875
  },
  {
    'c': 1.2313,
    'h': 1.289675,
    'l': 1.208825
  },
  {
    'c': 1.288275,
    'h': 1.289125,
    'l': 1.2196
  },
  {
    'c': 1.27005,
    'h': 1.29605,
    'l': 1.2482
  },
  {
    'c': 1.2051,
    'h': 1.27005,
    'l': 1.194075
  },
  {
    'c': 1.184,
    'h': 1.23095,
    'l': 1.1747
  }
]

export default function getStorybookState (): FTXState {
  const hasBalances = boolean('FTX: has balances?', false)
  const conversion = boolean('FTX: conversion in progress?', false)
  const conversionData: ConversionData = {
    from: 'BAT',
    to: 'BTC',
    quantity: 19.24242,
    isSubmitting: boolean('FTX: is submitting conversion?', false),
    complete: boolean('FTX: conversion complete?', false),
    quote: boolean('FTX: is fetching conversion quote?', false)
      ? undefined
      : { quoteId: '123', cost: '19.24242', proceeds: '24242.5', price: '8869' }
  }
  return {
    hasInitialized: boolean('FTX: has initialized?', true),
    isConnected: boolean('FTX: is connected?', false),
    ftxHost: 'ftx.com',
    balances: hasBalances ? balances : {},
    balanceTotal: hasBalances ? Object.values(balances).reduce((a, b) => a + b, 0) : null,
    marketData: marketData,
    currencyNames: ['BAT', 'BTC', 'USD'],
    currentView: ViewType[select('FTX: page', viewTypeArray, ViewType[ViewType.OptIn])],
    assetDetail: boolean('FTX: show asset detail', false)
      ? {
        chartData: select('FTX: chart data', { None: undefined, Error: 'Error getting chart', Data: chartData as any }, chartData),
        currencyName: 'BAT',
        marketData: batMarketData
      }
      : null,
    conversionInProgress: conversion
      ? conversionData
      : undefined
  }
}
