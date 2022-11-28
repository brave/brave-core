// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { createApi } from '@reduxjs/toolkit/query/react'

// types
import {
  BraveWallet,
  SupportedCoinTypes,
  SupportedTestNetworks,
  WalletInfoBase
} from '../../constants/types'
import { IsEip1559Changed } from '../constants/action_types'

// entity adaptors
import {
  networkEntityAdapter,
  networkEntityInitalState,
  NetworkEntityState
} from './entity-adaptors/network-entity-adaptor'
import {
  AccountInfoEntityState,
  accountInfoEntityAdaptor,
  accountInfoEntityAdaptorInitialState
} from './entity-adaptors/account-info-entity-adaptor'

// utils
import { cacher } from '../../utils/query-cache-utils'
import getAPIProxy from '../async/bridge'
import WalletApiProxy from '../wallet_api_proxy'

export function createWalletApi (
  getProxy: () => WalletApiProxy = () => getAPIProxy()
) {
  const walletApi = createApi({
    reducerPath: 'walletApi',
    baseQuery: () => {
      return { data: getProxy() }
    },
    tagTypes: [
      ...cacher.defaultTags,
      'Network',
      'TestnetsEnabled',
      'SelectedCoin',
      'WalletInfo',
      'AccountInfos',
      'DefaultAccountAddresses',
      'SelectedAccountAddress',
      'ChainIdForCoinType',
      'SelectedChainId'
    ],
    endpoints: ({ mutation, query }) => ({
      //
      // Accounts & Wallet Info
      //
      getWalletInfoBase: query<WalletInfoBase, void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          const { walletHandler } = baseQuery(undefined).data
          const walletInfo: WalletInfoBase = await walletHandler.getWalletInfo()
          return {
            data: walletInfo
          }
        },
        providesTags: ['WalletInfo']
      }),
      getAccountInfosRegistry: query<AccountInfoEntityState, void>({
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          const walletInfo: WalletInfoBase = await dispatch(
            walletApi.endpoints.getWalletInfoBase.initiate()
          ).unwrap()
          const accountInfos: BraveWallet.AccountInfo[] = walletInfo.accountInfos
          return {
            data: accountInfoEntityAdaptor.setAll(
              accountInfoEntityAdaptorInitialState,
              accountInfos
            )
          }
        },
        providesTags: cacher.providesRegistry('AccountInfos')
      }),
      getDefaultAccountAddresses: query<string[], void>({
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          const { keyringService } = baseQuery(undefined).data // apiProxy

          // Get default account addresses for each CoinType
          const defaultAccountAddresses = await Promise.all(SupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
            const chainId: string = await dispatch(walletApi.endpoints.getChainIdForCoin.initiate(coin)).unwrap()
            const defaultAccount = coin === BraveWallet.CoinType.FIL
              ? await keyringService.getFilecoinSelectedAccount(chainId)
              : await keyringService.getSelectedAccount(coin)
            return defaultAccount.address
          }))

          // remove empty addresses
          const filteredDefaultAccountAddresses = defaultAccountAddresses
            .filter((account: string | null): account is string => account !== null && account !== '')

          return {
            data: filteredDefaultAccountAddresses
          }
        },
        providesTags: ['DefaultAccountAddresses']
      }),
      setSelectedAccount: mutation<string, {
        address: string
        coin: BraveWallet.CoinType
      }>({
        async queryFn ({ address, coin }, api, extraOptions, baseQuery) {
          const { keyringService } = baseQuery(undefined).data // apiProxy
          await keyringService.setSelectedAccount(address, coin)
          return {
            data: address
          }
        },
        invalidatesTags: ['SelectedAccountAddress']
      }),
      getSelectedAccountAddress: query<string, void>({
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          const { keyringService } = baseQuery(undefined).data // apiProxy

          const selectedCoin: number = await dispatch(walletApi.endpoints.getSelectedCoin.initiate()).unwrap()

          let selectedAddress: string | null = null
          if (selectedCoin === BraveWallet.CoinType.FIL) {
            const chainId: string = await dispatch(walletApi.endpoints.getChainIdForCoin.initiate(selectedCoin)).unwrap()
            selectedAddress = (await keyringService.getFilecoinSelectedAccount(chainId)).address
          } else {
            selectedAddress = (await keyringService.getSelectedAccount(selectedCoin)).address
          }

          const accountsRegistry: AccountInfoEntityState = await dispatch(walletApi.endpoints.getAccountInfosRegistry.initiate()).unwrap()
          const fallbackAccount = accountsRegistry[accountsRegistry.ids[0]]

          if (
            // If the selected address is null, set the selected account address to the fallback address
            selectedAddress === null || selectedAddress === '' ||
            // If a user has already created an wallet but then chooses to restore
            // a different wallet, getSelectedAccount still returns the previous wallets
            // selected account.
            // This check looks to see if the returned selectedAccount exist in the accountInfos
            // payload, if not it will setSelectedAccount to the fallback address
            !accountsRegistry.ids.find((accountId) => String(accountId).toLowerCase() === selectedAddress?.toLowerCase())
          ) {
            await dispatch(walletApi.endpoints.setSelectedAccount.initiate(fallbackAccount))
            return {
              data: fallbackAccount.address
            }
          }

          return {
            data: selectedAddress
          }
        },
        providesTags: ['SelectedAccountAddress']
      }),
      //
      // Networks
      //
      getAllNetworks: query<NetworkEntityState, void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          try {
            const apiProxy = baseQuery(undefined).data
            const networksList = await fetchNetworksList(apiProxy)
            const normalizedNetworksState = networkEntityAdapter.setAll(
              networkEntityInitalState,
              networksList
            )
            return {
              data: normalizedNetworksState
            }
          } catch (error) {
            return {
              error: `Unable to fetch AllNetworks ${error}`
            }
          }
        },
        providesTags: cacher.providesRegistry('Network')
      }),
      getChainIdForCoin: query<string, BraveWallet.CoinType>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          const { jsonRpcService } = baseQuery(undefined).data // apiProxy
          const { chainId } = await jsonRpcService.getChainId(arg)
          return {
            data: chainId
          }
        },
        providesTags: cacher.cacheByIdArg('ChainIdForCoinType')
      }),
      getSelectedChainId: query<string, void>({
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          const selectedCoin: number = await dispatch(walletApi.endpoints.getSelectedCoin.initiate()).unwrap()
          const chainId: string = await dispatch(walletApi.endpoints.getChainIdForCoin.initiate(selectedCoin)).unwrap()
          return {
            data: chainId
          }
        },
        providesTags: ['SelectedChainId']
      }),
      getIsTestNetworksEnabled: query<boolean, void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          try {
            const { braveWalletService } = baseQuery(undefined).data

            const { isEnabled: testNetworksEnabled } =
              await braveWalletService.getShowWalletTestNetworks()

            return {
              data: testNetworksEnabled
            }
          } catch (error) {
            return {
              error: `Unable to fetch isTestNetworksEnabled ${error}`
            }
          }
        },
        providesTags: ['TestnetsEnabled']
      }),
      getSelectedCoin: query<BraveWallet.CoinType, void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          try {
            const apiProxy = baseQuery(undefined).data
            const { braveWalletService } = apiProxy
            const { coin } = await braveWalletService.getSelectedCoin()
            return { data: coin }
          } catch (error) {
            return {
              error: `Unable to fetch selectedCoin: ${error}`
            }
          }
        },
        providesTags: ['SelectedCoin']
      }),
      setSelectedCoin: mutation<BraveWallet.CoinType, BraveWallet.CoinType>({
        queryFn (coinTypeArg, api, extraOptions, baseQuery) {
          try {
            const { braveWalletService } = baseQuery(undefined).data
            braveWalletService.setSelectedCoin(coinTypeArg)
            return { data: coinTypeArg }
          } catch (error) {
            return {
              error: `Unable to mutate selectedCoin: ${error}`
            }
          }
        },
        invalidatesTags: ['SelectedCoin']
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
  useSetSelectedCoinMutation,
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
  const { isFilecoinEnabled, isSolanaEnabled } =
    await walletHandler.getWalletInfo()

  // Get isTestNetworkEnabled
  const { isEnabled: testNetworksEnabled } =
    await braveWalletService.getShowWalletTestNetworks()

  // Get All Networks
  const filteredSupportedCoinTypes = SupportedCoinTypes.filter((coin) => {
    // MULTICHAIN: While we are still in development for FIL and SOL,
    // we will not use their networks unless enabled by brave://flags
    return (
      (coin === BraveWallet.CoinType.FIL && isFilecoinEnabled) ||
      (coin === BraveWallet.CoinType.SOL && isSolanaEnabled) ||
      coin === BraveWallet.CoinType.ETH
    )
  })

  const networkLists = await Promise.all(
    filteredSupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
      const { networks } = await jsonRpcService.getAllNetworks(coin)
      return networks
    })
  )

  const flattenedNetworkList = networkLists.flat(1)

  const { chainId: defaultEthChainId } = await jsonRpcService.getChainId(
    BraveWallet.CoinType.ETH
  )
  const { chainIds: hiddenEthNetworkList } =
    await jsonRpcService.getHiddenNetworks(BraveWallet.CoinType.ETH)

  const networkList = flattenedNetworkList
    .filter((network) => {
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
