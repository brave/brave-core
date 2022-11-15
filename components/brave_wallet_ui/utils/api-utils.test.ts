// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { getTokenParam, getFlattenedAccountBalances } from './api-utils'
import { mockAccount } from '../common/constants/mocks'
import { mockEthToken, mockBasicAttentionToken } from '../stories/mock-data/mock-asset-options'

const ethToken = mockEthToken
const batToken = mockBasicAttentionToken

describe('Check token param', () => {
  test('Value should return contract address', () => {
    expect(getTokenParam(batToken)).toEqual('0x0D8775F648430679A709E98d2b0Cb6250d2887EF')
  })

  test('Value should return symbol', () => {
    expect(getTokenParam({
      ...batToken,
      contractAddress: ''
    })).toEqual('bat')
  })

  test('Value should return coingeckoId', () => {
    expect(getTokenParam({
      ...batToken,
      coingeckoId: 'mockCoingeckoId'
    })).toEqual('mockCoingeckoId')
  })
})

const mockUserVisibleTokenOptions = [
  ethToken,
  batToken
]
const mockAccounts = [
  {
    ...mockAccount,
    nativeBalanceRegistry: {
      '0x1': '238699740940532526'
    },
    tokenBalanceRegistry: {
      [batToken.contractAddress.toLowerCase()]: '0'
    }
  },
  {
    ...mockAccount,
    nativeBalanceRegistry: {
      '0x1': '80573000000000000'
    },
    tokenBalanceRegistry: {
      [batToken.contractAddress.toLowerCase()]: '0'
    }
  }
]

const expectedResult = [
  {
    balance: 319272740940532500,
    token: ethToken
  },
  {
    balance: 0,
    token: batToken
  }
]

describe('Check Flattened Account Balances', () => {
  test('Value should return an Empty Array', () => {
    expect(getFlattenedAccountBalances([], mockUserVisibleTokenOptions)).toEqual([])
  })

  test('Value should return a Flattened Account Balance List', () => {
    expect(getFlattenedAccountBalances(mockAccounts, mockUserVisibleTokenOptions)).toEqual(expectedResult)
  })
})
