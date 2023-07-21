// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { getPriceIdForToken } from './api-utils'
import { mockBasicAttentionToken } from '../stories/mock-data/mock-asset-options'

const batToken = mockBasicAttentionToken

describe('Check getPriceIdForToken()', () => {
  test('Value should return contract address', () => {
    expect(getPriceIdForToken(batToken)).toEqual(
      '0x0d8775f648430679a709e98d2b0cb6250d2887ef'
    )
  })

  test('Value should return symbol', () => {
    expect(
      getPriceIdForToken({
        ...batToken,
        contractAddress: ''
      })
    ).toEqual('bat')
  })

  test('Value should return coingeckoId', () => {
    expect(
      getPriceIdForToken({
        ...batToken,
        coingeckoId: 'mockCoingeckoId'
      })
    ).toEqual('mockcoingeckoid')
  })
})
