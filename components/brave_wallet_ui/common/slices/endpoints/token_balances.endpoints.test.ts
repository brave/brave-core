// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

// queries
import { useGetTokenBalancesRegistryQuery } from '../api.slice'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'
import { mockEthAccount } from '../../../stories/mock-data/mock-wallet-accounts'
import { mockEthMainnet } from '../../../stories/mock-data/mock-networks'
import {
  createEmptyTokenBalancesRegistry,
  getBalanceFromRegistry,
  setBalance
} from '../../../utils/balance-utils'

// mocks
import {
  mockMoonCatNFT,
  mockSplNft
} from '../../../stories/mock-data/mock-asset-options'
import {
  mockSolanaAccount,
  mockSolanaMainnetNetwork
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
  isShielded: false
})
setBalance({
  accountId: mockSolanaAccount.accountId,
  balance: '1',
  chainId: mockSolanaMainnetNetwork.chainId,
  coinType: mockSolanaMainnetNetwork.coin,
  contractAddress: mockSplNft.contractAddress,
  tokenId: mockSplNft.tokenId,
  tokenBalancesRegistry: mockedTokenBalancesRegistry,
  isShielded: false
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
            userAssets: [mockMoonCatNFT, mockSplNft]
          }
        )

        const { result } = renderHook(
          () =>
            useGetTokenBalancesRegistryQuery({
              accountIds: [
                mockEthAccount.accountId,
                mockSolanaAccount.accountId
              ],
              networks: [mockEthMainnet, mockSolanaMainnetNetwork],
              useAnkrBalancesFeature,
              isSpamRegistry: false
            }),
          renderHookOptionsWithMockStore(store)
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
            isShielded: false
          })
        ).toBe('1')
        expect(
          getBalanceFromRegistry({
            accountUniqueId: mockEthAccount.accountId.uniqueKey,
            chainId: mockEthMainnet.chainId,
            coin: mockEthMainnet.coin,
            contractAddress: mockMoonCatNFT.contractAddress,
            tokenId: '0x1111',
            registry: registry!,
            isShielded: false
          })
        ).toBe('0')
        expect(
          getBalanceFromRegistry({
            accountUniqueId: mockSolanaAccount.accountId.uniqueKey,
            chainId: mockSolanaMainnetNetwork.chainId,
            coin: mockSolanaMainnetNetwork.coin,
            contractAddress: mockSplNft.contractAddress,
            tokenId: mockSplNft.tokenId,
            registry: registry!,
            isShielded: false
          })
        ).toBe('1')
        expect(
          getBalanceFromRegistry({
            accountUniqueId: mockSolanaAccount.accountId.uniqueKey,
            chainId: mockSolanaMainnetNetwork.chainId,
            coin: mockSolanaMainnetNetwork.coin,
            contractAddress: 'wt1t1111111111111111111',
            tokenId: '',
            registry: registry!,
            isShielded: false
          })
        ).toBe('0')
      }
    )
  })
})
