// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

import {
  createApi
} from '@reduxjs/toolkit/query/react'
import { REHYDRATE } from 'redux-persist'

// types
import { StringWithAutocomplete } from '../../constants/types'

// utils
import { cacher } from '../../utils/query-cache-utils'

import { baseQueryFunction } from '../async/base-query-cache'
import { CombinedState } from '@reduxjs/toolkit'
import { AnyAction } from 'redux'


type ApiRehydrationAction = Partial<{
  type: typeof REHYDRATE
  key: StringWithAutocomplete<'walletApi'>
  payload:
    | CombinedState<{}>
    | undefined
}>

const API_TAG_TYPES = [
  ...cacher.defaultTags,
  'AccountInfos',
  'AccountTokenCurrentBalance',
  'BraveRewards-Enabled',
  'BraveRewards-ExternalWallet',
  'BraveRewards-RewardsBalance',
  'CombinedTokenBalanceForAllAccounts',
  'DefaultFiatCurrency',
  'ERC721Metadata',
  'GasEstimation1559',
  'KnownBlockchainTokens',
  'Network',
  'NftDiscoveryEnabledStatus',
  'SolanaEstimatedFees',
  'TokenBalancesForChainId',
  'TokenSpotPrices',
  'Transactions',
  'TransactionSimulationsEnabled',
  'UserBlockchainTokens',
  'WalletInfo',
] as const

export type ApiTagTypeName = typeof API_TAG_TYPES[number]

/**
 * Creates an api to use as a base for adding endpoints
 * endpoints can be added via `.injectEndpoints(endpoints)`
*/
export function createWalletApiBase() {
  return createApi({
    reducerPath: 'walletApi',
    baseQuery: baseQueryFunction,
    tagTypes: API_TAG_TYPES,
    endpoints: ({ mutation, query }) => ({}),
    extractRehydrationInfo(
      action: AnyAction | ApiRehydrationAction,
      { reducerPath }
    ) {
      try {
        // attempt to rehydrate from persisted query cache
        // whenever the REHYDRATE action is dispatched
        if (
          action.type === REHYDRATE &&
          !!action?.payload &&
          action?.key === reducerPath
        ) {
          return action.payload
        }
        return undefined
      } catch (error) {
        console.error(`error during rehydration: ${error}`)
        return undefined
      }
    },
  })
}

export type WalletApiBase = ReturnType<typeof createWalletApiBase>
export const walletApiBase: WalletApiBase = createWalletApiBase()

export type WalletApiEndpointBuilder = Parameters<
  WalletApiBase['injectEndpoints']
>[0]['endpoints']

export type WalletApiEndpointBuilderParams =
  Parameters<WalletApiEndpointBuilder>[0]
