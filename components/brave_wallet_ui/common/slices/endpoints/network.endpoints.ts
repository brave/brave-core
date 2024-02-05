// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'

// constants
import { BraveWallet } from '../../../constants/types'

// types
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'
import { IsEip1559Changed } from '../../constants/action_types'
import { NetworksRegistry } from '../entities/network.entity'
import { ACCOUNT_TAG_IDS } from './account.endpoints'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'

export const NETWORK_TAG_IDS = {
  REGISTRY: 'REGISTRY',
  SELECTED: 'SELECTED',
  SWAP_SUPPORTED: 'SWAP_SUPPORTED',
  CUSTOM_ASSET_SUPPORTED: 'CUSTOM_ASSET_SUPPORTED'
} as const

interface IsEip1559ChangedMutationArg {
  id: string
  isEip1559: boolean
}

export const networkEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    // queries
    /** Gets all networks, regardless of which coin-types are enabled */
    getAllKnownNetworks: query<BraveWallet.NetworkInfo[], void>({
      queryFn: async (_arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)

          const { isBitcoinEnabled } =
            cache.walletInfo || (await cache.getWalletInfo())

          const { networks: ethNetworks } =
            await api.jsonRpcService.getAllNetworks(BraveWallet.CoinType.ETH)
          const { networks: solNetworks } =
            await api.jsonRpcService.getAllNetworks(BraveWallet.CoinType.SOL)
          const { networks: filNetworks } =
            await api.jsonRpcService.getAllNetworks(BraveWallet.CoinType.FIL)
          const btcNetworks = isBitcoinEnabled
            ? (
                await api.jsonRpcService.getAllNetworks(
                  BraveWallet.CoinType.BTC
                )
              ).networks
            : []

          return {
            data: [
              ...ethNetworks,
              ...solNetworks,
              ...filNetworks,
              ...btcNetworks
            ]
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to fetch all known networks',
            error
          )
        }
      },
      providesTags: ['Network']
    }),
    getNetworksRegistry: query<NetworksRegistry, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          return {
            data: await cache.getNetworksRegistry()
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to fetch Networks Registry ${error}`,
            error
          )
        }
      },
      providesTags: (res, err, arg) =>
        err
          ? ['UNKNOWN_ERROR']
          : [{ type: 'Network', id: NETWORK_TAG_IDS.REGISTRY }]
    }),
    getSwapSupportedNetworks: query<BraveWallet.NetworkInfo[], void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const {
            data: { swapService },
            cache
          } = baseQuery(undefined)

          const networksRegistry = await cache.getNetworksRegistry()

          const chainIdsWithSupportFlags = await mapLimit(
            networksRegistry.ids,
            10,
            async (chainId: string) => {
              const { result } = await swapService.isSwapSupported(
                chainId.toString()
              )
              return {
                chainId,
                supported: result
              }
            }
          )

          const swapChainIds = chainIdsWithSupportFlags
            .filter(({ supported }) => !!supported)
            .map((net) => net.chainId.toString())

          return {
            data: getEntitiesListFromEntityState(networksRegistry, swapChainIds)
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to get Swap-Supported Networks',
            error
          )
        }
      },
      providesTags: [{ type: 'Network', id: NETWORK_TAG_IDS.SWAP_SUPPORTED }]
    }),
    invalidateSelectedChain: mutation<boolean, void>({
      queryFn: () => {
        return { data: true }
      },
      invalidatesTags: [{ type: 'Network', id: NETWORK_TAG_IDS.SELECTED }]
    }),
    getSelectedChain: query<BraveWallet.NetworkInfo | null, void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { network } = await api.braveWalletService //
            .getNetworkForSelectedAccountOnActiveOrigin()
          return {
            data: network
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to fetch the currently selected chain`,
            error
          )
        }
      },
      providesTags: (res, err) =>
        err
          ? ['UNKNOWN_ERROR']
          : [{ type: 'Network', id: NETWORK_TAG_IDS.SELECTED }]
    }),
    // mutations
    hideNetworks: mutation<
      boolean,
      Array<Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>>
    >({
      queryFn: async (chains, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)

          await mapLimit(
            chains,
            10,
            async (
              chain: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
            ) => {
              const { success } = await api.jsonRpcService.addHiddenNetwork(
                chain.coin,
                chain.chainId
              )

              if (!success) {
                throw new Error('jsonRpcService.addHiddenNetwork failed')
              }
            }
          )

          cache.clearNetworksRegistry()

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to hide networks: ${chains
              .map((chain) => chain.chainId)
              .join()}`,
            error
          )
        }
      },
      invalidatesTags: (result, error) => ['Network']
    }),
    restoreNetworks: mutation<
      boolean,
      Array<Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>>
    >({
      queryFn: async (chains, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)

          await mapLimit(
            chains,
            10,
            async (
              chain: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
            ) => {
              const { success } = await api.jsonRpcService.removeHiddenNetwork(
                chain.coin,
                chain.chainId
              )

              if (!success) {
                throw new Error('jsonRpcService.removeHiddenNetwork failed')
              }
            }
          )

          cache.clearNetworksRegistry()

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to unhide networks: ${chains
              .map((chain) => chain.chainId)
              .join()}`,
            error
          )
        }
      },
      invalidatesTags: (result, error) => ['Network']
    }),
    setNetwork: mutation<
      {
        needsAccountForNetwork?: boolean
        selectedAccountId?: BraveWallet.AccountId
      },
      Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
    >({
      queryFn: async (
        { chainId, coin },
        { endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const {
            data: { braveWalletService },
            cache
          } = baseQuery(undefined)

          cache.clearSelectedAccount()
          const { accountId: selectedAccountId } =
            await braveWalletService.ensureSelectedAccountForChain(
              coin,
              chainId
            )
          if (!selectedAccountId) {
            return {
              data: { needsAccountForNetwork: true }
            }
          }

          const {
            success //
          } = await braveWalletService //
            .setNetworkForSelectedAccountOnActiveOrigin(chainId)
          if (!success) {
            throw new Error(
              'braveWalletService.' +
                'SetNetworkForSelectedAccountOnActiveOrigin failed'
            )
          }

          return {
            data: { selectedAccountId }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to change selected network to: (chainId: ${
              chainId //
            }, coin: ${
              coin //
            })`,
            error
          )
        }
      },
      invalidatesTags: (result, error, { coin }) => [
        { type: 'Network', id: NETWORK_TAG_IDS.SELECTED },
        { type: 'AccountInfos', id: ACCOUNT_TAG_IDS.SELECTED }
      ]
    }),
    isEip1559Changed: mutation<IsEip1559ChangedMutationArg, IsEip1559Changed>({
      queryFn: async (arg, _, __, baseQuery) => {
        // invalidate base cache of networks
        baseQuery(undefined).cache.clearNetworksRegistry()

        const { chainId, isEip1559 } = arg
        return {
          data: { id: chainId, isEip1559 }
        }
      },
      invalidatesTags: ['Network']
    }),
    refreshNetworkInfo: mutation<boolean, void>({
      queryFn: async (arg, api, extraOptions, baseQuery) => {
        // invalidate base cache of networks
        baseQuery(undefined).cache.clearNetworksRegistry()
        // invalidates tags
        return {
          data: true
        }
      },
      // refresh networks & selected network
      invalidatesTags: ['Network']
    })
  }
}
