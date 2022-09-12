// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'
import Amount from './amount'

export const findAssetPrice = (spotPrices: BraveWallet.AssetPrice[], symbol: string) => {
  return spotPrices.find(
    (token) => token.fromAsset.toLowerCase() === symbol.toLowerCase()
  )?.price ?? ''
}

export const computeFiatAmount = (
  spotPrices: BraveWallet.AssetPrice[],
  asset: {
    value: string
    symbol: string
    decimals: number
  }
): Amount => {
  const price = findAssetPrice(spotPrices, asset.symbol)

  if (!price || !asset.value) {
    return Amount.empty()
  }

  return new Amount(asset.value)
    .divideByDecimals(asset.decimals) // Wei → ETH conversion
    .times(price)
}
