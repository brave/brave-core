// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  TokenBalancesRegistry //
} from '../common/slices/entities/token-balance.entity'

// utils
import {
  checkIfTokensMatch,
  checkIfTokenNeedsNetworkIcon,
  searchNftCollectionsAndGetTotalNftsFound,
  getTokenCollectionName,
  getAssetIdKey,
  isTokenWatchOnly
} from './asset-utils'
import { getAccountBalancesKey } from './balance-utils'

// mocks
import { mockEthMainnet } from '../stories/mock-data/mock-networks'
import {
  mockEthToken,
  mockBasicAttentionToken,
  mockMoonCatNFT,
  mockErc721Token
} from '../stories/mock-data/mock-asset-options'
import { mockAccounts } from '../stories/mock-data/mock-wallet-accounts'

const ethToken = mockEthToken
const batToken = mockBasicAttentionToken
const nftTokenOne = mockMoonCatNFT
const nftTokenTwo = { ...mockMoonCatNFT, tokenId: '0x42a7' }

const mockCollectionAssetsRegistry = {
  'MoonCatsRescue': [mockMoonCatNFT, nftTokenTwo],
  'Invisible Friends': [mockErc721Token]
}

const mockCollectionAssetIdsRegistry = {
  'MoonCatsRescue': [
    getAssetIdKey({ ...mockMoonCatNFT, tokenId: '' }),
    getAssetIdKey({ ...nftTokenTwo, tokenId: '' })
  ],
  'Invisible Friends': [getAssetIdKey({ ...mockErc721Token, tokenId: '' })]
}

const mockUserTokenBalancesRegistry: TokenBalancesRegistry = {
  accounts: {
    [getAccountBalancesKey(mockAccounts[0].accountId)]: {
      chains: {
        [mockMoonCatNFT.chainId]: {
          tokenBalances: {
            [getAssetIdKey(mockMoonCatNFT)]: '1'
          }
        }
      }
    }
  }
}

const mockEmptyUserTokenBalancesRegistry: TokenBalancesRegistry = {
  accounts: {
    [getAccountBalancesKey(mockAccounts[0].accountId)]: {
      chains: {
        [mockMoonCatNFT.chainId]: {
          tokenBalances: {
            [getAssetIdKey(mockMoonCatNFT)]: '0'
          }
        }
      }
    }
  }
}

describe('Check if tokens match', () => {
  test('Comparing BAT to BAT, should match.', () => {
    expect(checkIfTokensMatch(batToken, batToken)).toBeTruthy()
  })

  test('Comparing BAT to ETH, should not match.', () => {
    expect(checkIfTokensMatch(batToken, ethToken)).toBeFalsy()
  })

  test('Comparing NFTs with the same tokenId, should match.', () => {
    expect(checkIfTokensMatch(nftTokenOne, nftTokenOne)).toBeTruthy()
  })

  test('Comparing NFTs with different tokenIds, should not match.', () => {
    expect(checkIfTokensMatch(nftTokenOne, nftTokenTwo)).toBeFalsy()
  })
})

describe('Check if token needs Network icon', () => {
  test('Comparing ETH to Ethereum Network, should return false', () => {
    expect(
      checkIfTokenNeedsNetworkIcon(mockEthMainnet, ethToken.contractAddress)
    ).toBeFalsy()
  })

  test('Comparing BAT to Ethereum Network, should return true', () => {
    expect(
      checkIfTokenNeedsNetworkIcon(mockEthMainnet, batToken.contractAddress)
    ).toBeTruthy()
  })
})

describe('searchNftCollectionsAndGetTotalNftsFound', () => {
  test('should return the correct count of assets in the registry', () => {
    const moonCatsCollectionAsset = {
      ...mockMoonCatNFT,
      name: 'MoonCatsRescue'
    }

    expect(
      searchNftCollectionsAndGetTotalNftsFound(
        'moon',
        [],
        mockCollectionAssetsRegistry
      ).totalNftsFound
    ).toBe(0)

    expect(
      searchNftCollectionsAndGetTotalNftsFound(
        'moon',
        [moonCatsCollectionAsset],
        mockCollectionAssetsRegistry
      ).totalNftsFound
    ).toBe(2)

    expect(
      searchNftCollectionsAndGetTotalNftsFound(
        'nothing',
        [moonCatsCollectionAsset],
        mockCollectionAssetsRegistry
      ).totalNftsFound
    ).toBe(0)

    expect(
      searchNftCollectionsAndGetTotalNftsFound(
        'sun',
        [{ ...mockMoonCatNFT, name: 'SunCatsRescue' }],
        mockCollectionAssetsRegistry
      ).totalNftsFound
    ).toBe(0)
  })
})

describe('getTokenCollectionName', () => {
  it('should return the correct collection name', () => {
    const collectionNames = ['MoonCatsRescue', 'Invisible Friends']
    const moonCatCollectionName = getTokenCollectionName(
      collectionNames,
      mockCollectionAssetIdsRegistry,
      mockMoonCatNFT
    )
    const invisibleFriendCollectionName = getTokenCollectionName(
      collectionNames,
      mockCollectionAssetIdsRegistry,
      mockErc721Token
    )
    expect(moonCatCollectionName).toBe('MoonCatsRescue')
    expect(invisibleFriendCollectionName).toBe('Invisible Friends')
  })
})

describe('isTokenWatchOnly', () => {
  it('should check if the token has a balance', () => {
    expect(
      isTokenWatchOnly(
        mockMoonCatNFT,
        mockAccounts,
        mockUserTokenBalancesRegistry,
        mockUserTokenBalancesRegistry
      )
    ).toBe(false)
    expect(
      isTokenWatchOnly(
        mockMoonCatNFT,
        mockAccounts,
        mockEmptyUserTokenBalancesRegistry,
        mockEmptyUserTokenBalancesRegistry
      )
    ).toBe(true)
  })
})
