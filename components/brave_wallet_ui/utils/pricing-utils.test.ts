// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// import { BraveWallet } from '../constants/types'
// import Amount from './amount'

import { mockAssetPrices } from '../common/constants/mocks'
import { mockBasicAttentionToken } from '../stories/mock-data/mock-asset-options'
import {
  computeFiatAmount,
  findAssetPrice
} from './pricing-utils'

describe('findAssetPrice', () => {
  it('should find the price of a coin from within a list of asset prices', () => {
    const { symbol } = mockBasicAttentionToken
    expect(
      findAssetPrice(
        mockAssetPrices,
        symbol
      )
    ).toBe('0.88')
  })
})

describe('computeFiatAmount', () => {
  it('should find and convert the fiat value of a token', () => {
    const { symbol, decimals } = mockBasicAttentionToken

    expect(
      computeFiatAmount(
        mockAssetPrices,
        { symbol, decimals, value: '20' }
      ).format()
    ).toBe('0.0000000000000000176')
  })
})
