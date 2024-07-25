// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// import { BraveWallet } from '../constants/types'
// import Amount from './amount'

import { mockSpotPriceRegistry } from '../common/constants/mocks'
import { mockBasicAttentionToken } from '../stories/mock-data/mock-asset-options'
import {
  computeFiatAmount,
  getTokenPriceFromRegistry,
  getTokenPriceAmountFromRegistry,
  computeFiatAmountToAssetValue,
  getPriceIdForToken
} from './pricing-utils'

describe('getTokenPriceFromRegistry', () => {
  it('should get the price of a coin from the spot prices registry', () => {
    expect(
      getTokenPriceFromRegistry(mockSpotPriceRegistry, mockBasicAttentionToken)
        ?.price
    ).toBe('0.88')
  })
})

describe('getTokenPriceAmountFromRegistry', () => {
  it('should get the price amount from spot prices registry', () => {
    expect(
      getTokenPriceAmountFromRegistry(
        mockSpotPriceRegistry,
        mockBasicAttentionToken
      ).formatAsFiat()
    ).toBe('0.88')
  })
})

describe('computeFiatAmount', () => {
  it('should find and convert the fiat value of a token', () => {
    expect(
      computeFiatAmount({
        spotPriceRegistry: mockSpotPriceRegistry,
        value: '20',
        token: mockBasicAttentionToken
      }).format()
    ).toBe('0.0000000000000000176')
  })
})

describe('computeFiatAmountToAssetValue', () => {
  it('should find and convert the fiat amoount to token value', () => {
    expect(
      computeFiatAmountToAssetValue({
        spotPriceRegistry: mockSpotPriceRegistry,
        value: '200',
        token: mockBasicAttentionToken
      }).format(6)
    ).toBe('227.273')
  })
})

describe('Check getPriceIdForToken()', () => {
  test('Value should return contract address', () => {
    expect(getPriceIdForToken(mockBasicAttentionToken)).toEqual(
      '0x0d8775f648430679a709e98d2b0cb6250d2887ef'
    )
  })

  test('Value should return symbol', () => {
    expect(
      getPriceIdForToken({
        ...mockBasicAttentionToken,
        contractAddress: ''
      })
    ).toEqual('bat')
  })

  test('Value should return coingeckoId', () => {
    expect(
      getPriceIdForToken({
        ...mockBasicAttentionToken,
        coingeckoId: 'mockCoingeckoId'
      })
    ).toEqual('mockcoingeckoid')
  })
})
