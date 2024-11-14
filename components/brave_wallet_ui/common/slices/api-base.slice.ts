// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createApi } from '@reduxjs/toolkit/query/react'

// utils
import { cacher } from '../../utils/query-cache-utils'

import { baseQueryFunction } from '../async/base-query-cache'

// TODO: These should eventually get refactored into a single tag with a type of
// `'TokenBalances'` with various IDs to simplify clearing cached balances
// across queries
const balancesTags = [
  'AccountTokenCurrentBalance',
  'TokenBalancesForChainId',
  'TokenBalances',
  'HardwareAccountDiscoveryBalance'
] as const

/**
 * Creates an api to use as a base for adding endpoints
 * endpoints can be added via `.injectEndpoints(endpoints)`
 */
export function createWalletApiBase() {
  return createApi({
    reducerPath: 'walletApi',
    baseQuery: baseQueryFunction,
    tagTypes: [
      ...cacher.defaultTags,
      ...balancesTags,
      'AccountInfos',
      'DefaultFiatCurrency',
      'NftMetadata',
      'SolanaEstimatedFees',
      'GasEstimation1559',
      'KnownBlockchainTokens',
      'Network',
      'TokenSpotPrices',
      'PriceHistory',
      'PricesHistory',
      'Transactions',
      'TransactionSimulationsOptIn',
      'UserBlockchainTokens',
      'NftDiscoveryEnabledStatus',
      'BraveRewards-Info',
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
      'TokenInfo',
      'EthTokenDecimals',
      'EthTokenSymbol',
      'EnsOffchainLookupEnabled',
      'NameServiceAddress',
      'IsWalletBackedUp',
      'ConnectedAccounts',
      'DefaultEthWallet',
      'DefaultSolWallet',
      'IsMetaMaskInstalled',
      'IsPrivateWindow',
      'PendingAddChainRequests',
      'PendingSwitchChainRequests',
      'PendingDecryptRequest',
      'PendingEncryptRequest',
      'PendingSignSolTransactionsRequests',
      'PinnableNftIds',
      'PendingSignMessageRequests',
      'PendingSignMessageErrors',
      'ActiveOrigin',
      'MeldFiatCurrencies',
      'MeldCryptoCurrencies',
      'DefaultCountryCode',
      'MeldCountries',
      'MeldServiceProviders',
      'MeldCryptoQuotes',
      'MeldPaymentMethods',
      'MeldWidget',
      'ZCashAccountInfo',
      'IsShieldingAvailable'
    ],
    endpoints: ({ mutation, query }) => ({})
  })
}

export const ACCOUNT_TAG_IDS = {
  REGISTRY: 'REGISTRY',
  SELECTED: 'SELECTED'
} as const

export const NETWORK_TAG_IDS = {
  HIDDEN: 'HIDDEN',
  LIST: 'LIST',
  MAINNETS: 'MAINNETS',
  OFF_RAMPS: 'OFF_RAMP',
  ON_RAMPS: 'ON_RAMP',
  REGISTRY: 'REGISTRY',
  SELECTED: 'SELECTED',
  SWAP_SUPPORTED: 'SWAP_SUPPORTED',
  VISIBLE: 'VISIBLE'
} as const

export const TOKEN_TAG_IDS = {
  REGISTRY: 'REGISTRY'
} as const

export type WalletApiBase = ReturnType<typeof createWalletApiBase>
export const walletApiBase: WalletApiBase = createWalletApiBase()

export type WalletApiEndpointBuilder = Parameters<
  WalletApiBase['injectEndpoints']
>[0]['endpoints']

export type WalletApiEndpointBuilderParams =
  Parameters<WalletApiEndpointBuilder>[0]
