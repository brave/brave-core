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
import { IsEip1559ChangedMutationArg } from '../api.slice'
import { NetworksRegistry } from '../entities/network.entity'
import { ACCOUNT_TAG_IDS } from './account.endpoints'

export const NETWORK_TAG_IDS = {
  REGISTRY: 'REGISTRY',
  SELECTED: 'SELECTED',
  SWAP_SUPPORTED: 'SWAP_SUPPORTED',
} as const

export const networkEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
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
    getSwapSupportedNetworkIds: query<string[], void>({
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
            data: swapChainIds
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
