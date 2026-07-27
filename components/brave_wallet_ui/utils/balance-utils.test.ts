// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  createEmptyTokenBalancesRegistry,
  getBalance,
  getPercentAmount,
  setBalance,
} from './balance-utils'

// types
import { BraveWallet } from '../constants/types'

// mocks
import { mockAccount } from '../common/constants/mocks'
import {
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token,
  mockERC20Token,
} from '../stories/mock-data/mock-asset-options'

describe('getBalance', () => {
  it('gets a balance of a token for a given account', () => {
    const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
    setBalance({
      accountId: mockAccount.accountId,
      chainId: mockBasicAttentionToken.chainId,
      contractAddress: mockBasicAttentionToken.contractAddress.toLowerCase(),
      balance: '123',
      tokenBalancesRegistry,
      coinType: mockBasicAttentionToken.coin,
      tokenId: '',
      zcashTokenType: BraveWallet.ZCashTokenType.kNone,
    })

    expect(
      getBalance(
        mockAccount.accountId,
        mockBasicAttentionToken,
        tokenBalancesRegistry,
      ),
    ).toBe('123')
  })

  it('returns zero balance if address is unknown', () => {
    const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
    setBalance({
      accountId: { uniqueKey: '0xdeadbeef' },
      chainId: mockBasicAttentionToken.chainId,
      contractAddress: mockBasicAttentionToken.contractAddress.toLowerCase(),
      balance: '123',
      tokenBalancesRegistry,
      coinType: mockBasicAttentionToken.coin,
      tokenId: '',
      zcashTokenType: BraveWallet.ZCashTokenType.kNone,
    })

    expect(
      getBalance(
        mockAccount.accountId,
        mockBinanceCoinErc20Token,
        tokenBalancesRegistry,
      ),
    ).toBe('0')
  })

  it('returns zero balance if chainId is unknown', () => {
    const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
    setBalance({
      accountId: mockAccount.accountId,
      chainId: '0xdeadbeef',
      contractAddress: mockBasicAttentionToken.contractAddress.toLowerCase(),
      balance: '123',
      tokenBalancesRegistry,
      coinType: mockBasicAttentionToken.coin,
      tokenId: '',
      zcashTokenType: BraveWallet.ZCashTokenType.kNone,
    })

    expect(
      getBalance(
        mockAccount.accountId,
        mockBinanceCoinErc20Token,
        tokenBalancesRegistry,
      ),
    ).toBe('0')
  })

  it('returns zero balance if token contract is unknown', () => {
    const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
    setBalance({
      accountId: mockAccount.accountId,
      chainId: mockBasicAttentionToken.chainId,
      contractAddress: '0xdeadbeef',
      balance: '123',
      tokenBalancesRegistry,
      coinType: mockBasicAttentionToken.coin,
      tokenId: '',
      zcashTokenType: BraveWallet.ZCashTokenType.kNone,
    })

    expect(
      getBalance(
        mockAccount.accountId,
        mockBinanceCoinErc20Token,
        tokenBalancesRegistry,
      ),
    ).toBe('0')
  })

  it('returns empty string if tokenBalancesRegistry is undefined', () => {
    expect(
      getBalance(mockAccount.accountId, mockBinanceCoinErc20Token, undefined),
    ).toBe('')
  })
})

describe('getPercentAmount', () => {
  it.each([
    ['25% of 1 ETH', '1000000000000000000', 0.25, '0.25'],
    ['50% of 1 ETH', '1000000000000000000', 0.5, '0.5'],
    ['75% of 1 ETH', '1000000000000000000', 0.75, '0.75'],
    ['100% of 1 ETH', '1000000000000000000', 1, '1'],
    [
      '100% of 1.000000000000000001 ETH',
      '1000000000000000001',
      1,
      '1.000000000000000001',
    ], // 1.000000000000000001 ---(100%)---> 1.000000000000000001
    ['100% of 50.297939 ETH', '50297939000000000000', 1, '50.297939'], // 50.297939 ---(100%)---> 50.297939
    ['25% of 0.0001 ETH', '100000000000000', 0.25, '0.000025'], // 0.0001 ---(25%)---> 0.000025
  ])(
    'should compute %s correctly',
    (_, balance: string, percent, expected: string) => {
      const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
      setBalance({
        accountId: mockAccount.accountId,
        chainId: mockERC20Token.chainId,
        contractAddress: mockERC20Token.contractAddress,
        balance,
        tokenBalancesRegistry,
        coinType: mockERC20Token.coin,
        tokenId: '',
        zcashTokenType: BraveWallet.ZCashTokenType.kNone,
      })

      expect(
        getPercentAmount(
          mockERC20Token,
          mockAccount.accountId,
          percent,
          tokenBalancesRegistry,
        ),
      ).toBe(expected)
    },
  )
})
