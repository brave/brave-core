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
    setBalance({
      accountId: mockAccount.accountId,
      chainId: mockBasicAttentionToken.chainId,
      contractAddress: mockBasicAttentionToken.contractAddress.toLowerCase(),
      balance: '123',
      tokenBalancesRegistry,
      coinType: mockBasicAttentionToken.coin,
      tokenId: '',
      isShielded: false
    })

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
    setBalance({
      accountId: { uniqueKey: '0xdeadbeef' },
      chainId: mockBasicAttentionToken.chainId,
      contractAddress: mockBasicAttentionToken.contractAddress.toLowerCase(),
      balance: '123',
      tokenBalancesRegistry,
      coinType: mockBasicAttentionToken.coin,
      tokenId: '',
      isShielded: false
    })

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
    setBalance({
      accountId: mockAccount.accountId,
      chainId: '0xdeadbeef',
      contractAddress: mockBasicAttentionToken.contractAddress.toLowerCase(),
      balance: '123',
      tokenBalancesRegistry,
      coinType: mockBasicAttentionToken.coin,
      tokenId: '',
      isShielded: false
    })

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
    setBalance({
      accountId: mockAccount.accountId,
      chainId: mockBasicAttentionToken.chainId,
      contractAddress: '0xdeadbeef',
      balance: '123',
      tokenBalancesRegistry,
      coinType: mockBasicAttentionToken.coin,
      tokenId: '',
      isShielded: false
    })

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
      setBalance({
        accountId: mockAccount.accountId,
        chainId: mockERC20Token.chainId,
        contractAddress: mockERC20Token.contractAddress,
        balance,
        tokenBalancesRegistry,
        coinType: mockERC20Token.coin,
        tokenId: '',
        isShielded: false
      })

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
  const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()

  // Native ETH
  setBalance({
    accountId: mockEthAccountInfo.accountId,
    chainId: BraveWallet.MAINNET_CHAIN_ID,
    contractAddress: '',
    balance,
    tokenBalancesRegistry,
    coinType: BraveWallet.CoinType.ETH,
    tokenId: '',
    isShielded: false
  })

  // Native MATIC
  setBalance({
    accountId: mockEthAccountInfo.accountId,
    chainId: BraveWallet.POLYGON_MAINNET_CHAIN_ID,
    contractAddress: '',
    balance,
    tokenBalancesRegistry,
    coinType: BraveWallet.CoinType.ETH,
    tokenId: '',
    isShielded: false
  })

  // Native SOL
  setBalance({
    accountId: mockSolanaAccount.accountId,
    chainId: BraveWallet.SOLANA_MAINNET,
    contractAddress: '',
    balance,
    tokenBalancesRegistry,
    coinType: BraveWallet.CoinType.SOL,
    tokenId: '',
    isShielded: false
  })

  // NATIVE FIL
  setBalance({
    accountId: mockFilecoinAccount.accountId,
    chainId: BraveWallet.FILECOIN_MAINNET,
    contractAddress: '',
    balance,
    tokenBalancesRegistry,
    coinType: BraveWallet.CoinType.FIL,
    tokenId: '',
    isShielded: false
  })

  // NATIVE BTC
  setBalance({
    accountId: mockBitcoinAccount.accountId,
    chainId: BraveWallet.BITCOIN_MAINNET,
    contractAddress: '',
    balance,
    tokenBalancesRegistry,
    coinType: BraveWallet.CoinType.BTC,
    tokenId: '',
    isShielded: false
  })

  // NATIVE TESTNET BTC
  setBalance({
    accountId: mockBitcoinTestAccount.accountId,
    chainId: BraveWallet.BITCOIN_TESTNET,
    contractAddress: '',
    balance,
    tokenBalancesRegistry,
    coinType: BraveWallet.CoinType.BTC,
    tokenId: '',
    isShielded: false
  })

  // NATIVE ZEC
  setBalance({
    accountId: mockZecAccount.accountId,
    chainId: BraveWallet.Z_CASH_MAINNET,
    contractAddress: '',
    balance,
    tokenBalancesRegistry,
    coinType: BraveWallet.CoinType.ZEC,
    tokenId: '',
    isShielded: false
  })

  return tokenBalancesRegistry
}

const mockAccountIds = mockAccounts.map((account) => account.accountId)

describe('getActiveWalletCount', () => {
  it('should return nothing with empty input', () => {
    const registry = createEmptyTokenBalancesRegistry()
    expect(getActiveWalletCount([], registry, false)).toStrictEqual({})
  })

  it('should return no active accounts when zero balance', () => {
    const registry = createMockRegistry('0')

    expect(
      getActiveWalletCount(
        mockAccountIds,
        registry,
        false //
      )
    ).toStrictEqual({
      [BraveWallet.CoinType.BTC]: 0,
      [BraveWallet.CoinType.ETH]: 0,
      [BraveWallet.CoinType.FIL]: 0,
      [BraveWallet.CoinType.ZEC]: 0,
      [BraveWallet.CoinType.SOL]: 0
    })
  })

  it('should skip testnets with balance by default', () => {
    const tokenBalancesRegistry = createMockRegistry('0')

    // SEPOLIA ETH
    setBalance({
      accountId: mockEthAccountInfo.accountId,
      chainId: BraveWallet.SEPOLIA_CHAIN_ID,
      contractAddress: '',
      balance: '1',
      tokenBalancesRegistry,
      coinType: BraveWallet.CoinType.ETH,
      tokenId: '',
      isShielded: false
    })

    // TESTNET BTC
    setBalance({
      accountId: mockBitcoinTestAccount.accountId,
      chainId: BraveWallet.BITCOIN_TESTNET,
      contractAddress: '',
      balance: '1',
      tokenBalancesRegistry,
      coinType: BraveWallet.CoinType.BTC,
      tokenId: '',
      isShielded: false
    })

    expect(
      getActiveWalletCount(mockAccountIds, tokenBalancesRegistry, false)
    ).toStrictEqual({
      [BraveWallet.CoinType.ETH]: 0,
      [BraveWallet.CoinType.SOL]: 0,
      [BraveWallet.CoinType.FIL]: 0,
      [BraveWallet.CoinType.ZEC]: 0,
      [BraveWallet.CoinType.BTC]: 0
    })
  })

  it('should include testnets with flag', () => {
    const tokenBalancesRegistry = createMockRegistry('0')

    // SEPOLIA ETH
    setBalance({
      accountId: mockEthAccountInfo.accountId,
      chainId: BraveWallet.SEPOLIA_CHAIN_ID,
      contractAddress: '',
      balance: '1',
      tokenBalancesRegistry,
      coinType: BraveWallet.CoinType.ETH,
      tokenId: '',
      isShielded: false
    })

    // TESTNET BTC
    setBalance({
      accountId: mockBitcoinTestAccount.accountId,
      chainId: BraveWallet.BITCOIN_TESTNET,
      contractAddress: '',
      balance: '1',
      tokenBalancesRegistry,
      coinType: BraveWallet.CoinType.BTC,
      tokenId: '',
      isShielded: false
    })

    expect(
      getActiveWalletCount(mockAccountIds, tokenBalancesRegistry, true)
    ).toStrictEqual({
      [BraveWallet.CoinType.ETH]: 1,
      [BraveWallet.CoinType.SOL]: 0,
      [BraveWallet.CoinType.FIL]: 0,
      [BraveWallet.CoinType.ZEC]: 0,
      [BraveWallet.CoinType.BTC]: 1
    })
  })

  it('should report active accounts', () => {
    const registry = createMockRegistry('1')

    expect(getActiveWalletCount(mockAccountIds, registry, true)).toStrictEqual({
      [BraveWallet.CoinType.ETH]: 1,
      [BraveWallet.CoinType.SOL]: 1,
      [BraveWallet.CoinType.FIL]: 1,
      [BraveWallet.CoinType.ZEC]: 1,
      [BraveWallet.CoinType.BTC]: 2 // mainnet and testnet accounts
    })
  })

  it('should report many active accounts', () => {
    const tokenBalancesRegistry = createMockRegistry('1')

    const mockEthAccountInfo2 = { ...mockEthAccountInfo }
    mockEthAccountInfo2.accountId.uniqueKey = 'mockEthAccountInfo2'

    // MOCK ETH ERC TOKEN
    setBalance({
      accountId: mockEthAccountInfo2.accountId,
      chainId: BraveWallet.MAINNET_CHAIN_ID,
      contractAddress: '0x1234contract',
      balance: '1',
      tokenBalancesRegistry,
      coinType: BraveWallet.CoinType.ETH,
      tokenId: '',
      isShielded: false
    })

    const mockZecAccount2 = { ...mockZecAccount }
    mockZecAccount2.accountId.uniqueKey = 'mockZecAccount2'

    // NATIVE ZEC
    setBalance({
      accountId: mockZecAccount2.accountId,
      chainId: BraveWallet.Z_CASH_MAINNET,
      contractAddress: '',
      balance: '1',
      tokenBalancesRegistry,
      coinType: BraveWallet.CoinType.ZEC,
      tokenId: '',
      isShielded: false
    })

    expect(
      getActiveWalletCount(
        [
          ...mockAccountIds,
          mockEthAccountInfo2.accountId,
          mockZecAccount2.accountId
        ],
        tokenBalancesRegistry,
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
