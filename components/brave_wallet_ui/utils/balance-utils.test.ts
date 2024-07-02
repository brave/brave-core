// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'

import {
  createEmptyTokenBalancesRegistry,
  getActiveWalletCount,
  getBalance,
  getPercentAmount,
  setBalance
} from './balance-utils'

// mocks
import {
  mockAccount,
  mockBitcoinAccount,
  mockBitcoinTestAccount,
  mockEthAccountInfo,
  mockFilecoinAccount,
  mockSolanaAccount,
  mockZecAccount
} from '../common/constants/mocks'
import {
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token,
  mockERC20Token
} from '../stories/mock-data/mock-asset-options'

describe('getBalance', () => {
  it('gets a balance of a token for a given account', () => {
    const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
    setBalance(
      mockAccount.accountId,
      mockBasicAttentionToken.chainId,
      mockBasicAttentionToken.contractAddress.toLowerCase(),
      '123',
      tokenBalancesRegistry
    )

    expect(
      getBalance(
        mockAccount.accountId,
        mockBasicAttentionToken,
        tokenBalancesRegistry
      )
    ).toBe('123')
  })

  it('returns zero balance if address is unknown', () => {
    const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
    setBalance(
      { uniqueKey: '0xdeadbeef' },
      mockBasicAttentionToken.chainId,
      mockBasicAttentionToken.contractAddress.toLowerCase(),
      '123',
      tokenBalancesRegistry
    )

    expect(
      getBalance(
        mockAccount.accountId,
        mockBinanceCoinErc20Token,
        tokenBalancesRegistry
      )
    ).toBe('0')
  })

  it('returns zero balance if chainId is unknown', () => {
    const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
    setBalance(
      mockAccount.accountId,
      '0xdeadbeef',
      mockBasicAttentionToken.contractAddress.toLowerCase(),
      '123',
      tokenBalancesRegistry
    )

    expect(
      getBalance(
        mockAccount.accountId,
        mockBinanceCoinErc20Token,
        tokenBalancesRegistry
      )
    ).toBe('0')
  })

  it('returns zero balance if token contract is unknown', () => {
    const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
    setBalance(
      mockAccount.accountId,
      mockBasicAttentionToken.chainId,
      '0xdeadbeef',
      '123',
      tokenBalancesRegistry
    )

    expect(
      getBalance(
        mockAccount.accountId,
        mockBinanceCoinErc20Token,
        tokenBalancesRegistry
      )
    ).toBe('0')
  })

  it('returns empty string if tokenBalancesRegistry is undefined', () => {
    expect(
      getBalance(mockAccount.accountId, mockBinanceCoinErc20Token, undefined)
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
      '1.000000000000000001'
    ], // 1.000000000000000001 ---(100%)---> 1.000000000000000001
    ['100% of 50.297939 ETH', '50297939000000000000', 1, '50.297939'], // 50.297939 ---(100%)---> 50.297939
    ['25% of 0.0001 ETH', '100000000000000', 0.25, '0.000025'] // 0.0001 ---(25%)---> 0.000025
  ])(
    'should compute %s correctly',
    (_, balance: string, percent, expected: string) => {
      const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
      setBalance(
        mockAccount.accountId,
        mockERC20Token.chainId,
        mockERC20Token.contractAddress,
        balance,
        tokenBalancesRegistry
      )

      expect(
        getPercentAmount(
          mockERC20Token,
          mockAccount.accountId,
          percent,
          tokenBalancesRegistry
        )
      ).toBe(expected)
    }
  )
})

const mockAccounts = [
  mockEthAccountInfo,
  mockSolanaAccount,
  mockFilecoinAccount,
  mockBitcoinAccount,
  mockBitcoinTestAccount,
  mockZecAccount
]

const createMockRegistry = (balance: string) => {
  const registry = createEmptyTokenBalancesRegistry()
  setBalance(
    mockEthAccountInfo.accountId,
    BraveWallet.MAINNET_CHAIN_ID,
    '',
    balance,
    registry
  )
  setBalance(
    mockEthAccountInfo.accountId,
    BraveWallet.POLYGON_MAINNET_CHAIN_ID,
    '',
    balance,
    registry
  )
  setBalance(
    mockEthAccountInfo.accountId,
    BraveWallet.SEPOLIA_CHAIN_ID,
    '',
    balance,
    registry
  )

  setBalance(
    mockSolanaAccount.accountId,
    BraveWallet.SOLANA_MAINNET,
    '',
    balance,
    registry
  )

  setBalance(
    mockFilecoinAccount.accountId,
    BraveWallet.FILECOIN_MAINNET,
    '',
    balance,
    registry
  )

  setBalance(
    mockBitcoinAccount.accountId,
    BraveWallet.BITCOIN_MAINNET,
    '',
    balance,
    registry
  )
  setBalance(
    mockBitcoinTestAccount.accountId,
    BraveWallet.BITCOIN_TESTNET,
    '',
    balance,
    registry
  )

  setBalance(
    mockZecAccount.accountId,
    BraveWallet.Z_CASH_MAINNET,
    '',
    balance,
    registry
  )

  return registry
}

describe('getActiveWalletCount', () => {
  it('should return nothing with empty input', () => {
    const registry = createEmptyTokenBalancesRegistry()
    expect(getActiveWalletCount([], registry, false)).toStrictEqual({})
  })

  it('should return no active accounts when zero balance', () => {
    const registry = createMockRegistry('0')

    expect(getActiveWalletCount(mockAccounts, registry, false)).toStrictEqual({
      [BraveWallet.CoinType.BTC]: 0,
      [BraveWallet.CoinType.ETH]: 0,
      [BraveWallet.CoinType.FIL]: 0,
      [BraveWallet.CoinType.ZEC]: 0,
      [BraveWallet.CoinType.SOL]: 0
    })
  })

  it('should skip testnets with balance by default', () => {
    const registry = createMockRegistry('0')

    setBalance(
      mockEthAccountInfo.accountId,
      BraveWallet.SEPOLIA_CHAIN_ID,
      '',
      '1',
      registry
    )
    setBalance(
      mockBitcoinTestAccount.accountId,
      BraveWallet.BITCOIN_TESTNET,
      '',
      '1',
      registry
    )

    expect(getActiveWalletCount(mockAccounts, registry, false)).toStrictEqual({
      [BraveWallet.CoinType.ETH]: 0,
      [BraveWallet.CoinType.SOL]: 0,
      [BraveWallet.CoinType.FIL]: 0,
      [BraveWallet.CoinType.ZEC]: 0,
      [BraveWallet.CoinType.BTC]: 0
    })
  })

  it('should include testnets with flag', () => {
    const registry = createMockRegistry('0')

    setBalance(
      mockEthAccountInfo.accountId,
      BraveWallet.SEPOLIA_CHAIN_ID,
      '',
      '1',
      registry
    )
    setBalance(
      mockBitcoinTestAccount.accountId,
      BraveWallet.BITCOIN_TESTNET,
      '',
      '1',
      registry
    )

    expect(getActiveWalletCount(mockAccounts, registry, true)).toStrictEqual({
      [BraveWallet.CoinType.ETH]: 1,
      [BraveWallet.CoinType.SOL]: 0,
      [BraveWallet.CoinType.FIL]: 0,
      [BraveWallet.CoinType.ZEC]: 0,
      [BraveWallet.CoinType.BTC]: 1
    })
  })

  it('should report active accounts', () => {
    const registry = createMockRegistry('1')

    expect(getActiveWalletCount(mockAccounts, registry, true)).toStrictEqual({
      [BraveWallet.CoinType.ETH]: 1,
      [BraveWallet.CoinType.SOL]: 1,
      [BraveWallet.CoinType.FIL]: 1,
      [BraveWallet.CoinType.ZEC]: 1,
      [BraveWallet.CoinType.BTC]: 2 // mainnet and testnet accounts
    })
  })

  it('should report many active accounts', () => {
    const registry = createMockRegistry('1')

    const mockEthAccountInfo2 = { ...mockEthAccountInfo }
    mockEthAccountInfo2.accountId.uniqueKey = 'mockEthAccountInfo2'
    setBalance(
      mockEthAccountInfo2.accountId,
      BraveWallet.MAINNET_CHAIN_ID,
      '0x1234contract',
      '1',
      registry
    )

    const mockZecAccount2 = { ...mockZecAccount }
    mockZecAccount2.accountId.uniqueKey = 'mockZecAccount2'
    setBalance(
      mockZecAccount2.accountId,
      BraveWallet.Z_CASH_MAINNET,
      '',
      '1',
      registry
    )

    expect(
      getActiveWalletCount(
        [...mockAccounts, mockEthAccountInfo2, mockZecAccount2],
        registry,
        true
      )
    ).toStrictEqual({
      [BraveWallet.CoinType.ETH]: 2,
      [BraveWallet.CoinType.SOL]: 1,
      [BraveWallet.CoinType.FIL]: 1,
      [BraveWallet.CoinType.ZEC]: 2,
      [BraveWallet.CoinType.BTC]: 2
    })
  })
})
