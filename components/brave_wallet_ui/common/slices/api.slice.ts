// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { createApi } from '@reduxjs/toolkit/query/react'

// types
import {
  BraveWallet,
  SupportedCoinTypes,
  SupportedTestNetworks
} from '../../constants/types'
import { IsEip1559Changed } from '../constants/action_types'

// entity adaptors
import {
  networkEntityAdapter,
  networkEntityInitalState,
  NetworkEntityState
} from './entity-adaptors/network-entity-adaptor'

// utils
import { cacher } from '../../utils/query-cache-utils'
import getAPIProxy from '../async/bridge'
import WalletApiProxy from '../wallet_api_proxy'

export function createWalletApi (getProxy: () => WalletApiProxy = () => getAPIProxy()) {
  const walletApi = createApi({
    reducerPath: 'walletApi',
    baseQuery: () => {
      return { data: getProxy() }
    },
    tagTypes: [
      ...cacher.defaultTags,
      'Network',
      'TestnetsEnabled',
      'SelectedCoin'
    ],
    endpoints: ({ mutation, query }) => ({
      //
      // Networks
      //
      getAllNetworks: query<NetworkEntityState, void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          const apiProxy = baseQuery(undefined).data
          const networksList = await fetchNetworksList(apiProxy)
          const normalizedNetworksState = networkEntityAdapter.setAll(
            networkEntityInitalState,
            networksList
          )
          return {
            data: normalizedNetworksState
          }
        },
        providesTags: cacher.providesRegistry('Network')
      }),
      getIsTestNetworksEnabled: query<boolean, void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          const { braveWalletService } = baseQuery(undefined).data

          const {
            isEnabled: testNetworksEnabled
          } = await braveWalletService.getShowWalletTestNetworks()

          return {
            data: testNetworksEnabled
          }
        },
        providesTags: ['TestnetsEnabled']
      }),
      getSelectedCoin: query<BraveWallet.CoinType, void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          const apiProxy = baseQuery(undefined).data
          const { braveWalletService } = apiProxy

          const { coin } = await braveWalletService.getSelectedCoin()

          return { data: coin }
        },
        providesTags: ['SelectedCoin']
      }),
      isEip1559Changed: mutation<{ id: string, isEip1559: boolean }, IsEip1559Changed>({
        async queryFn (arg) {
          const { chainId, isEip1559 } = arg
          // cache which chains are using EIP1559
          return {
            data: { id: chainId, isEip1559 } // invalidate the cache of the network with this chainId
          }
        },
        async onQueryStarted ({ chainId, isEip1559 }, { dispatch, queryFulfilled }) {
          // optimistic updates
          // try manually updating the cached network with the updated isEip1559 value
          const patchResult = dispatch(walletApi.util.updateQueryData('getAllNetworks', undefined, (draft) => {
            const draftNet = draft.entities[chainId]
            if (draftNet) {
              draftNet.isEip1559 = isEip1559
            }
          }))

          try {
            await queryFulfilled
          } catch {
            // undo the optimistic update if the mutation failed
            patchResult.undo()
          }
        },
        invalidatesTags: cacher.invalidatesList('Network')
      })
    })
  })

  return walletApi
}

export const walletApi = createWalletApi()
export const {
  middleware: walletApiMiddleware,
  reducer: walletApiReducer,
  reducerPath: walletApiReducerPath,
  // hooks
  useGetAllNetworksQuery,
  useGetIsTestNetworksEnabledQuery,
  useIsEip1559ChangedMutation,
  useLazyGetAllNetworksQuery,
  useLazyGetIsTestNetworksEnabledQuery,
  useGetSelectedCoinQuery
} = walletApi

//
// Internals
//

async function fetchNetworksList ({
  braveWalletService,
  jsonRpcService,
  walletHandler
}: WalletApiProxy) {
  const {
    isFilecoinEnabled, isSolanaEnabled
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

  const networkList = flattenedNetworkList.filter((network) => {
    if (!testNetworksEnabled) {
      return !SupportedTestNetworks.includes(network.chainId)
    }

    return !(
      network.coin === BraveWallet.CoinType.ETH &&
      network.chainId !== defaultEthChainId &&
      hiddenEthNetworkList.includes(network.chainId)
    )
  })

  return networkList
}
