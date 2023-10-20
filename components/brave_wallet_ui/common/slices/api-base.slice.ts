// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createApi } from '@reduxjs/toolkit/query/react'

// utils
import { cacher } from '../../utils/query-cache-utils'

import { baseQueryFunction } from '../async/base-query-cache'

/**
 * Creates an api to use as a base for adding endpoints
 * endpoints can be added via `.injectEndpoints(endpoints)`
*/
export function createWalletApiBase () {
  return createApi({
    reducerPath: 'walletApi',
    baseQuery: baseQueryFunction,
    tagTypes: [
      ...cacher.defaultTags,
      'AccountInfos',
      'AccountTokenCurrentBalance',
      'TokenBalancesForChainId',
      'TokenBalances',
      'HardwareAccountDiscoveryBalance',
      'DefaultFiatCurrency',
      'ERC721Metadata',
      'SolanaEstimatedFees',
      'GasEstimation1559',
      'KnownBlockchainTokens',
      'Network',
      'TokenSpotPrices',
      'PriceHistory',
      'PricesHistory',
      'Transactions',
      'TransactionSimulationsEnabled',
      'UserBlockchainTokens',
      'WalletInfo',
      'NftDiscoveryEnabledStatus',
      'BraveRewards-Enabled',
      'BraveRewards-RewardsBalance',
      'BraveRewards-ExternalWallet',
      'NFTPinningStatus',
      'NFTSPinningStatus',
      'AutoPinEnabled',
      'OnRampAssets',
      'OffRampAssets',
      'CoingeckoId',
      'TokenSuggestionRequests',
      'CoingeckoId',
      'AutoPinEnabled',
      'SimpleHashSpamNFTs',
      'LocalIPFSNodeStatus',
      'EthTokenDecimals',
      'EthTokenSymbol',
      'EnsOffchainLookupEnabled',
      'NameServiceAddress',
    ],
    endpoints: ({ mutation, query }) => ({})
  })
}

export type WalletApiBase = ReturnType<typeof createWalletApiBase>
export const walletApiBase: WalletApiBase = createWalletApiBase()

export type WalletApiEndpointBuilder = Parameters<
  WalletApiBase['injectEndpoints']
>[0]['endpoints']

export type WalletApiEndpointBuilderParams =
  Parameters<WalletApiEndpointBuilder>[0]
