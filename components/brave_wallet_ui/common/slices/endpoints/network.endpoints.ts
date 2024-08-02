// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'

// constants
import { BraveWallet } from '../../../constants/types'

// types
import {
  ACCOUNT_TAG_IDS,
  NETWORK_TAG_IDS,
  WalletApiEndpointBuilderParams
} from '../api-base.slice'

// utils
import {
  getHasPendingRequests,
  handleEndpointError
} from '../../../utils/api-utils'
import { NetworksRegistry, getNetworkId } from '../entities/network.entity'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import { LOCAL_STORAGE_KEYS } from '../../constants/local-storage-keys'
import {
  makeInitialFilteredOutNetworkKeys,
  parseJSONFromLocalStorage,
  setLocalStorageItem
} from '../../../utils/local-storage-utils'

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
          const { data: api } = baseQuery(undefined)

          const { networks } = await api.jsonRpcService.getAllNetworks()

          return {
            data: [...networks]
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
    getPendingAddChainRequest: query<BraveWallet.AddChainRequest | null, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          const { requests } =
            await api.jsonRpcService.getPendingAddChainRequests()

          return {
            data: requests.length ? requests[0] : null
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get pending "Add-Chain" requests',
            error
          )
        }
      },
      providesTags: ['PendingAddChainRequests']
    }),
    getPendingSwitchChainRequest: query<
      BraveWallet.SwitchChainRequest | null,
      void
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          const { requests } =
            await api.jsonRpcService.getPendingSwitchChainRequests()

          if (requests.length) {
            return {
              data: requests[0]
            }
          }
          return {
            data: null
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get pending Switch-Chain requests',
            error
          )
        }
      },
      providesTags: ['PendingSwitchChainRequests']
    }),
    // mutations
    hideNetworks: mutation<
      boolean,
      Array<Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>>
    >({
      queryFn: async (
        chains,
        { endpoint, dispatch },
        extraOptions,
        baseQuery
      ) => {
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

              const networkKey = getNetworkId(chain)

              // Get currently filtered-out network Keys in Local Storage
              const currentFilteredOutNetworkKeys = parseJSONFromLocalStorage(
                'FILTERED_OUT_PORTFOLIO_NETWORK_KEYS',
                makeInitialFilteredOutNetworkKeys()
              )

              // add network to hide list if not in the list already
              const newFilteredOutNetworkKeys =
                currentFilteredOutNetworkKeys.includes(networkKey)
                  ? currentFilteredOutNetworkKeys
                  : currentFilteredOutNetworkKeys.concat(networkKey)

              // Update filtered-out network keys in Local Storage
              setLocalStorageItem(
                LOCAL_STORAGE_KEYS.FILTERED_OUT_PORTFOLIO_NETWORK_KEYS,
                JSON.stringify(newFilteredOutNetworkKeys)
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
      invalidatesTags: (result, error) => [
        'Network',
        'TokenBalances',
        'TokenBalances',
        'AccountTokenCurrentBalance'
      ]
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
      invalidatesTags: [
        'Network',
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance'
      ]
    }),
    acknowledgePendingAddChainRequest: mutation<
      /**  success */
      true,
      { chainId: string; isApproved: boolean }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)

          if (arg.isApproved) {
            cache.clearNetworksRegistry()
          }

          api.jsonRpcService.addEthereumChainRequestCompleted(
            arg.chainId,
            arg.isApproved
          )

          // close the panel if there are no additional pending requests
          const hasPendingRequests = await getHasPendingRequests()

          if (!hasPendingRequests) {
            api.panelHandler?.closeUI()
          }

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to acknowledge pending "Add-Chain" request',
            error
          )
        }
      },
      invalidatesTags: (res, error, arg) =>
        arg.isApproved
          ? ['Network', 'PendingAddChainRequests', 'PendingSwitchChainRequests']
          : ['PendingAddChainRequests', 'PendingSwitchChainRequests']
    }),
    acknowledgeSwitchChainRequest: mutation<
      /** success */
      true,
      {
        requestId: string
        isApproved: boolean
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          api.jsonRpcService.notifySwitchChainRequestProcessed(
            arg.requestId,
            arg.isApproved
          )

          const hasPendingRequests = await getHasPendingRequests()

          if (!hasPendingRequests) {
            api.panelHandler?.closeUI()
          }

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to acknowledge Switch-Chain request',
            error
          )
        }
      },
      invalidatesTags: ['PendingSwitchChainRequests']
    })
  }
}
