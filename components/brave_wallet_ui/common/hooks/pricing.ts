/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// types
import { BraveWallet } from '../../constants/types'

// utils
import Amount from '../../utils/amount'
import { findAssetPrice, computeFiatAmount } from '../../utils/pricing-utils'

export default function usePricing (spotPrices: BraveWallet.AssetPrice[]) {
  const _findAssetPrice = React.useCallback((symbol: string) => {
    return findAssetPrice(spotPrices, symbol)
  }, [spotPrices])

  const _computeFiatAmount = React.useCallback((value: string, symbol: string, decimals: number): Amount => {
    return computeFiatAmount(spotPrices, { decimals, symbol, value })
  }, [spotPrices])

  return {
    computeFiatAmount: _computeFiatAmount,
    findAssetPrice: _findAssetPrice
  }
}
