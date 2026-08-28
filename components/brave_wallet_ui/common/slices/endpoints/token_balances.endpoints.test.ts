// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

// types
import { BraveWallet } from '../../../constants/types'

// queries
import { useGetTokenBalancesRegistryQuery } from '../api.slice'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore,
} from '../../../utils/test-utils'
import {
  mockEthAccount,
  mockPolkadotAccount,
} from '../../../stories/mock-data/mock-wallet-accounts'
import { mockEthMainnet } from '../../../stories/mock-data/mock-networks'
import {
  createEmptyTokenBalancesRegistry,
  getBalanceFromRegistry,
  setBalance,
} from '../../../utils/balance-utils'
import {
  TokenBalancesRegistry, //
} from '../entities/token-balance.entity'

// mocks
import {
  mockMoonCatNFT,
  mockSplNft,
} from '../../../stories/mock-data/mock-asset-options'
import {
  mockPolkadotMainnetNetwork,
  mockSolanaAccount,
  mockSolanaMainnetNetwork,
} from '../../constants/mocks'

/** Eth Account owns a MoonCat NFT, SOL account owns 1 SPL NFT */
const mockedTokenBalancesRegistry = createEmptyTokenBalancesRegistry()
setBalance({
  accountId: mockEthAccount.accountId,
  balance: '1',
  chainId: mockEthMainnet.chainId,
  coinType: mockEthMainnet.coin,
  contractAddress: mockMoonCatNFT.contractAddress,
  tokenId: mockMoonCatNFT.tokenId,
  tokenBalancesRegistry: mockedTokenBalancesRegistry,
  zcashTokenType: BraveWallet.ZCashTokenType.kNone,
})
setBalance({
  accountId: mockSolanaAccount.accountId,
  balance: '1',
  chainId: mockSolanaMainnetNetwork.chainId,
  coinType: mockSolanaMainnetNetwork.coin,
  contractAddress: mockSplNft.contractAddress,
  tokenId: mockSplNft.tokenId,
  tokenBalancesRegistry: mockedTokenBalancesRegistry,
  zcashTokenType: BraveWallet.ZCashTokenType.kNone,
})

const makePolkadotToken = ({
  assetId,
  symbol,
  decimals = 12,
}: {
  assetId?: number
  symbol: string
  decimals?: number
}) =>
  ({
    // DOT asset tokens are keyed by their (decimal) asset id; the native asset
    // has no contract address.
    contractAddress: assetId === undefined ? '' : String(assetId),
    name: symbol,
    symbol,
    decimals,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    chainId: mockPolkadotMainnetNetwork.chainId,
    coin: BraveWallet.CoinType.DOT,
    isCompressed: false,
    isErc20: false,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: BraveWallet.SPLTokenProgram.kUnsupported,
    isNft: false,
    isSpam: false,
    zcashTokenType: BraveWallet.ZCashTokenType.kNone,
    logo: '',
  }) as BraveWallet.BlockchainToken

const mockDotToken = makePolkadotToken({
  symbol: 'DOT',
  decimals: mockPolkadotMainnetNetwork.decimals,
})
// Asset id 0 is valid and must not be mistaken for the native asset.
const mockDotAssetZero = makePolkadotToken({ assetId: 0, symbol: 'XZERO' })
const mockDotAsset1984 = makePolkadotToken({ assetId: 1984, symbol: 'XBBC' })
const mockDotAssetEmpty = makePolkadotToken({ assetId: 31, symbol: 'XEMPTY' })

const polkadotTokens = [
  mockDotToken,
  mockDotAssetZero,
  mockDotAsset1984,
  mockDotAssetEmpty,
]

/** DOT account holds native DOT plus two of the three assets. */
const mockedPolkadotBalancesRegistry = createEmptyTokenBalancesRegistry()
for (const [token, balance] of [
  [mockDotToken, '10000000000'],
  [mockDotAssetZero, '5000000000000'],
  [mockDotAsset1984, '1234000000000'],
  [mockDotAssetEmpty, '0'],
] as const) {
  setBalance({
    accountId: mockPolkadotAccount.accountId,
    balance,
    chainId: mockPolkadotMainnetNetwork.chainId,
    coinType: BraveWallet.CoinType.DOT,
    contractAddress: token.contractAddress,
    tokenId: '',
    tokenBalancesRegistry: mockedPolkadotBalancesRegistry,
    zcashTokenType: BraveWallet.ZCashTokenType.kNone,
  })
}

const getPolkadotBalance = (
  registry: TokenBalancesRegistry,
  token: BraveWallet.BlockchainToken,
) =>
  getBalanceFromRegistry({
    accountUniqueId: mockPolkadotAccount.accountId.uniqueKey,
    chainId: mockPolkadotMainnetNetwork.chainId,
    coin: BraveWallet.CoinType.DOT,
    contractAddress: token.contractAddress,
    tokenId: '',
    registry,
    zcashTokenType: BraveWallet.ZCashTokenType.kNone,
  })

describe('token balances endpoints', () => {
  describe('getTokenBalancesRegistry', () => {
    it.each([true, false])(
      'should fetch nft balances regardless if ankr is enabled or not',
      async (useAnkrBalancesFeature) => {
        const store = createMockStore(
          {},
          {
            tokenBalanceRegistry: mockedTokenBalancesRegistry,
            networks: [mockEthMainnet, mockSolanaMainnetNetwork],
            accountInfos: [mockEthAccount, mockSolanaAccount],
            blockchainTokens: [mockMoonCatNFT, mockSplNft],
            userAssets: [mockMoonCatNFT, mockSplNft],
          },
        )

        const { result } = renderHook(
          () =>
            useGetTokenBalancesRegistryQuery({
              accountIds: [
                mockEthAccount.accountId,
                mockSolanaAccount.accountId,
              ],
              networks: [mockEthMainnet, mockSolanaMainnetNetwork],
              useAnkrBalancesFeature,
              isSpamRegistry: false,
            }),
          renderHookOptionsWithMockStore(store),
        )

        await waitFor(() => {
          expect(result.current.error).toBeFalsy()
          expect(result.current.isLoading).toBeFalsy()
          expect(result.current.data).toBeTruthy()
        })

        const { data: registry, isLoading, error } = result.current

        expect(isLoading).toBe(false)
        expect(error).not.toBeDefined()
        expect(registry).toBeDefined()
        expect(
          getBalanceFromRegistry({
            accountUniqueId: mockEthAccount.accountId.uniqueKey,
            chainId: mockEthMainnet.chainId,
            coin: mockEthMainnet.coin,
            contractAddress: mockMoonCatNFT.contractAddress,
            tokenId: mockMoonCatNFT.tokenId,
            registry: registry!,
            zcashTokenType: BraveWallet.ZCashTokenType.kNone,
          }),
        ).toBe('1')
        expect(
          getBalanceFromRegistry({
            accountUniqueId: mockEthAccount.accountId.uniqueKey,
            chainId: mockEthMainnet.chainId,
            coin: mockEthMainnet.coin,
            contractAddress: mockMoonCatNFT.contractAddress,
            tokenId: '0x1111',
            registry: registry!,
            zcashTokenType: BraveWallet.ZCashTokenType.kNone,
          }),
        ).toBe('0')
        expect(
          getBalanceFromRegistry({
            accountUniqueId: mockSolanaAccount.accountId.uniqueKey,
            chainId: mockSolanaMainnetNetwork.chainId,
            coin: mockSolanaMainnetNetwork.coin,
            contractAddress: mockSplNft.contractAddress,
            tokenId: mockSplNft.tokenId,
            registry: registry!,
            zcashTokenType: BraveWallet.ZCashTokenType.kNone,
          }),
        ).toBe('1')
        expect(
          getBalanceFromRegistry({
            accountUniqueId: mockSolanaAccount.accountId.uniqueKey,
            chainId: mockSolanaMainnetNetwork.chainId,
            coin: mockSolanaMainnetNetwork.coin,
            contractAddress: 'wt1t1111111111111111111',
            tokenId: '',
            registry: registry!,
            zcashTokenType: BraveWallet.ZCashTokenType.kNone,
          }),
        ).toBe('0')
      },
    )

    it('should fetch Polkadot native and asset balances', async () => {
      const store = createMockStore(
        {},
        {
          tokenBalanceRegistry: mockedPolkadotBalancesRegistry,
          networks: [mockPolkadotMainnetNetwork],
          accountInfos: [mockPolkadotAccount],
          blockchainTokens: polkadotTokens,
          userAssets: polkadotTokens,
        },
      )

      const { result } = renderHook(
        () =>
          useGetTokenBalancesRegistryQuery({
            accountIds: [mockPolkadotAccount.accountId],
            networks: [mockPolkadotMainnetNetwork],
            useAnkrBalancesFeature: false,
            isSpamRegistry: false,
          }),
        renderHookOptionsWithMockStore(store),
      )

      await waitFor(() => {
        expect(result.current.error).toBeFalsy()
        expect(result.current.isLoading).toBeFalsy()
        expect(result.current.data).toBeTruthy()
      })

      const { data: registry, error } = result.current
      expect(error).not.toBeDefined()

      // Native DOT comes from getAccountBalance.
      expect(getPolkadotBalance(registry!, mockDotToken)).toBe('10000000000')

      // Assets come from the batched getAssetAccountBalances call.
      expect(getPolkadotBalance(registry!, mockDotAssetZero)).toBe(
        '5000000000000',
      )
      expect(getPolkadotBalance(registry!, mockDotAsset1984)).toBe(
        '1234000000000',
      )

      // A zero balance is skipped rather than written to the registry.
      expect(getPolkadotBalance(registry!, mockDotAssetEmpty)).toBe('0')
    })
  })
})
