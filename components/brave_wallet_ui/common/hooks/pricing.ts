/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { BraveWallet } from '../../constants/types'
import { formatFiatBalance } from '../../utils/format-balances'

export default function usePricing (spotPrices: BraveWallet.AssetPrice[]) {
  const findAssetPrice = React.useCallback((symbol: string) => {
    return spotPrices.find(
      (token) => token.fromAsset.toLowerCase() === symbol.toLowerCase()
    )?.price ?? ''
  }, [spotPrices])

  const computeFiatAmount = React.useCallback((value: string, symbol: string, decimals: number) => {
    const price = findAssetPrice(symbol)

    if (!price || !value) {
      return ''
    }

    return formatFiatBalance(value, decimals, price)
  }, [findAssetPrice])

  return { computeFiatAmount, findAssetPrice }
}
