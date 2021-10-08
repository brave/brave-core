/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AssetPriceInfo } from '../../constants/types'

export default function usePricing (spotPrices: AssetPriceInfo[]) {
  return React.useCallback((symbol: string) => {
    return spotPrices.find(
      (token) => token.fromAsset.toLowerCase() === symbol.toLowerCase()
    )?.price ?? '0'
  }, [spotPrices])
}
