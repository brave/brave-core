// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// import { BraveWallet } from '../constants/types'
// import Amount from './amount'

import { mockAssetPrices } from '../common/constants/mocks'
import { mockBasicAttentionToken } from '../stories/mock-data/mock-asset-options'
import {
  computeFiatAmount,
  findAssetPrice,
  computeFiatAmountToAssetValue
} from './pricing-utils'

describe('findAssetPrice', () => {
  it('should find the price of a coin from within a list of asset prices', () => {
    const { symbol, chainId, contractAddress } = mockBasicAttentionToken
    expect(
      findAssetPrice(
        mockAssetPrices,
        symbol,
        contractAddress,
        chainId
      )
    ).toBe('0.88')
  })
})

describe('computeFiatAmount', () => {
  it('should find and convert the fiat value of a token', () => {
    const { symbol, decimals, contractAddress, chainId } = mockBasicAttentionToken

    expect(
      computeFiatAmount(
        mockAssetPrices,
        { symbol, decimals, value: '20', contractAddress, chainId }
      ).format()
    ).toBe('0.0000000000000000176')
  })
})

describe('computeFiatAmountToAssetValue', () => {
  it('should find and convert the fiat amoount to token value', () => {
    const { symbol, contractAddress, chainId } = mockBasicAttentionToken

    expect(
      computeFiatAmountToAssetValue(
        '200',
        mockAssetPrices,
        symbol,
        contractAddress,
        chainId
      ).format(6)
    ).toBe('227.273')
  })
})
