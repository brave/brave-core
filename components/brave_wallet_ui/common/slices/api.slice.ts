// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { createApi } from '@reduxjs/toolkit/query/react'

// types
import {
  BraveWallet,
  SupportedCoinTypes,
  SupportedTestNetworks
} from '../../constants/types'

// utils
import { cacher } from '../../utils/query-cache-utils'
import getAPIProxy from '../async/bridge'
import WalletApiProxy from '../wallet_api_proxy'

export type NetworkInfoWithId = BraveWallet.NetworkInfo & { id: string }

export function createWalletApi (getProxy: () => WalletApiProxy = () => getAPIProxy()) {
  return createApi({
    baseQuery: () => {
      return { data: getProxy() }
    },
    tagTypes: [...cacher.defaultTags, 'Network'],
    endpoints: (builder) => ({
      getAllNetworks: builder.query<NetworkInfoWithId[], void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          const {
            jsonRpcService,
            braveWalletService,
            walletHandler
          } = baseQuery(undefined).data

          const {
            isFilecoinEnabled,
            isSolanaEnabled
          } = await walletHandler.getWalletInfo()

          // Get isTestNetworkEnabled
          const {
            isEnabled: testNetworksEnabled
          } = await braveWalletService.getShowWalletTestNetworks()

          // Get All Networks
          const filteredSupportedCoinTypes = SupportedCoinTypes.filter(coin => {
            // MULTICHAIN: While we are still in development for FIL and SOL,
            // we will not use their networks unless enabled by brave://flags
            return (
              (coin === BraveWallet.CoinType.FIL && isFilecoinEnabled) ||
              (coin === BraveWallet.CoinType.SOL && isSolanaEnabled) ||
              coin === BraveWallet.CoinType.ETH
            )
          })

          const networkLists = await Promise.all(filteredSupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
            const { networks } = await jsonRpcService.getAllNetworks(coin)
            return networks
          }))

          const flattenedNetworkList = networkLists.flat(1)

          const { chainId: defaultEthChainId } = await jsonRpcService.getChainId(BraveWallet.CoinType.ETH)
          const { chainIds: hiddenEthNetworkList } = await jsonRpcService.getHiddenNetworks(BraveWallet.CoinType.ETH)

          const networkList: NetworkInfoWithId[] = flattenedNetworkList.filter((network) => {
            if (!testNetworksEnabled) {
              return !SupportedTestNetworks.includes(network.chainId)
            }

            return !(
              network.coin === BraveWallet.CoinType.ETH &&
              network.chainId !== defaultEthChainId &&
              hiddenEthNetworkList.includes(network.chainId)
            )
          }).map(net => ({
            id: net.chainId,
            ...net
          }))

          return {
            data: networkList
          }
        },
        providesTags: cacher.providesList('Network')
      })
    })
  })
}

export const walletApi = createWalletApi()
export const {
  middleware: walletApiMiddleware,
  reducer: walletApiReducer,
  reducerPath: walletApiReducerPath,
  // hooks
  useGetAllNetworksQuery,
  useLazyGetAllNetworksQuery
} = walletApi
