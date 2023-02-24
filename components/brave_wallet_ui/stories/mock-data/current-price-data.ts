// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { AssetPriceWithContractAndChainId } from '../../constants/types'

export const mockedCurrentPriceData = [
  {
    name: 'Bitcoin',
    symbol: 'BTC',
    usd: '56806.36',
    btc: '1',
    usdTimeframeChange: '2.3',
    btcTimeframeChange: '-0.1'

  },
  {
    name: 'Binance Coin',
    symbol: 'BNB',
    usd: '502.24',
    btc: '0.0098',
    usdTimeframeChange: '-4.6',
    btcTimeframeChange: '-4.8'
  },
  {
    name: 'Ethereum',
    symbol: 'ETH',
    usd: '2156.20',
    btc: '0.042',
    usdTimeframeChange: '0.2',
    btcTimeframeChange: '0.3'
  },
  {
    name: 'Basic Attention Token',
    symbol: 'BAT',
    usd: '1.34',
    btc: '0.000022',
    usdTimeframeChange: '2',
    btcTimeframeChange: '2.1'
  }
]

export const mockTransactionSpotPrices: AssetPriceWithContractAndChainId[] = [
  {
    fromAsset: 'ETH',
    toAsset: 'USD',
    price: '3300',
    assetTimeframeChange: '',
    contractAddress: '',
    chainId: '0x1'
  },
  {
    fromAsset: 'BAT',
    toAsset: 'USD',
    price: '0.85',
    assetTimeframeChange: '',
    contractAddress: '0x0D8775F648430679A709E98d2b0Cb6250d2887EF',
    chainId: '0x1'
  }
]
