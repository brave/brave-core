// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityId } from '@reduxjs/toolkit'
import { mapLimit } from 'async'

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'
import { SetUserAssetVisiblePayloadType } from '../../constants/action_types'
import type { BaseQueryCache } from '../../async/base-query-cache'

// utils
import { handleEndpointError } from '../../../utils/api-utils'
import { addChainIdToToken, getAssetIdKey } from '../../../utils/asset-utils'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import { cacher } from '../../../utils/query-cache-utils'
import { addLogoToToken } from '../../async/lib'
import {
  BlockchainTokenEntityAdaptorState,
  blockchainTokenEntityAdaptor,
  blockchainTokenEntityAdaptorInitialState
} from '../entities/blockchain-token.entity'
import { networkEntityAdapter } from '../entities/network.entity'

export const TOKEN_TAG_IDS = {
  REGISTRY: 'REGISTRY'
} as const

export const tokenEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getTokensRegistry: query<BlockchainTokenEntityAdaptorState, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const {
            cache,
            data: { blockchainRegistry }
          } = baseQuery(undefined)

          const networksRegistry = await cache.getNetworksRegistry()
          const networksList: BraveWallet.NetworkInfo[] =
            getEntitiesListFromEntityState(
              networksRegistry,
              networksRegistry.visibleIds
            )

          const tokenIdsByChainId: Record<string, string[]> = {}
          const tokenIdsByCoinType: Record<BraveWallet.CoinType, string[]> = {}

          const getTokensList = async () => {
            const tokenListsForNetworks = await mapLimit(
              networksList,
              10,
              async (network: BraveWallet.NetworkInfo) => {
                const networkId = networkEntityAdapter.selectId(network)

                const { tokens } = await blockchainRegistry.getAllTokens(
                  network.chainId,
                  network.coin
                )

                const fullTokensListForChain: //
                BraveWallet.BlockchainToken[] = await mapLimit(
                  tokens,
                  10,
                  async (token: BraveWallet.BlockchainToken) => {
                    return addChainIdToToken(
                      await addLogoToToken(token),
                      network.chainId
                    )
                  }
                )

                tokenIdsByChainId[networkId] =
                  fullTokensListForChain.map(getAssetIdKey)

                tokenIdsByCoinType[network.coin] = (
                  tokenIdsByCoinType[network.coin] || []
                ).concat(tokenIdsByChainId[networkId] || [])

                return fullTokensListForChain
              }
            )

            const flattenedTokensList = tokenListsForNetworks.flat(1)
            return flattenedTokensList
          }

          let flattenedTokensList = await getTokensList()

          // on startup, the tokens list returned from core may be empty
          const startDate = new Date()
          const timeoutSeconds = 5
          const timeoutMilliseconds = timeoutSeconds * 1000

          // retry until we have some tokens or the request takes too
          // long
          while (
            // empty list
            flattenedTokensList.length < 1 &&
            // try until timeout reached
            new Date().getTime() - startDate.getTime() < timeoutMilliseconds
          ) {
            flattenedTokensList = await getTokensList()
          }

          // return an error on timeout, so a retry can be attempted
          if (flattenedTokensList.length === 0) {
            throw new Error('No tokens found in tokens registry')
          }

          const tokensByChainIdRegistry = blockchainTokenEntityAdaptor.setAll(
            {
              ...blockchainTokenEntityAdaptorInitialState,
              idsByChainId: tokenIdsByChainId,
              idsByCoinType: tokenIdsByCoinType,
              visibleTokenIds: [],
              visibleTokenIdsByChainId: {},
              visibleTokenIdsByCoinType: {}
            },
            flattenedTokensList
          )

          return {
            data: tokensByChainIdRegistry
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to fetch Tokens Registry',
            error
          )
        }
      },
      providesTags: cacher.providesRegistry('KnownBlockchainTokens')
    }),
    getUserTokensRegistry: query<BlockchainTokenEntityAdaptorState, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          return {
            data: await baseQuery(undefined).cache.getUserTokensRegistry()
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to fetch UserTokens Registry',
            error
          )
        }
      },
      providesTags: (res, err) =>
        err
          ? ['UNKNOWN_ERROR']
          : [
              {
                type: 'UserBlockchainTokens',
                id: TOKEN_TAG_IDS.REGISTRY
              }
            ]
    }),
    addUserToken: mutation<{ id: EntityId }, BraveWallet.BlockchainToken>({
      queryFn: async (tokenArg, { dispatch }, extraOptions, baseQuery) => {
        const {
          cache,
          data: { braveWalletService }
        } = baseQuery(undefined)

        const result = await addUserToken({
          braveWalletService,
          cache,
          tokenArg
        })
        const tokenIdentifier = blockchainTokenEntityAdaptor.selectId(tokenArg)

        if (!result.success) {
          return {
            error: `Error adding user token: ${tokenIdentifier}`
          }
        }

        return {
          data: { id: tokenIdentifier }
        }
      },
      invalidatesTags: (_, __, tokenArg) => [
        { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
        { type: 'UserBlockchainTokens', id: getAssetIdKey(tokenArg) }
      ]
    }),
    removeUserToken: mutation<boolean, BraveWallet.BlockchainToken>({
      queryFn: async (tokenArg, { endpoint }, extraOptions, baseQuery) => {
        const {
          cache,
          data: { braveWalletService }
        } = baseQuery(undefined)

        cache.clearUserTokensRegistry()

        try {
          const deleteResult = await braveWalletService.removeUserAsset(
            tokenArg
          )
          if (!deleteResult.success) {
            throw new Error()
          }
          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to remove user asset: ${getAssetIdKey(tokenArg)}`,
            error
          )
        }
      },
      invalidatesTags: (_, __, tokenArg) => [
        { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
        { type: 'UserBlockchainTokens', id: getAssetIdKey(tokenArg) }
      ]
    }),
    updateUserToken: mutation<{ id: EntityId }, BraveWallet.BlockchainToken>({
      queryFn: async (
        tokenArg,
        { dispatch, endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const { cache, data: api } = baseQuery(undefined)
          const { braveWalletService } = api

          cache.clearUserTokensRegistry()

          const deleteResult = await braveWalletService.removeUserAsset(
            tokenArg
          )

          if (!deleteResult.success) {
            throw new Error('Unable to delete token')
          }

          const { success } = await addUserToken({
            braveWalletService,
            cache,
            tokenArg
          })

          if (!success) {
            throw new Error('Unable to add token')
          }

          return {
            data: {
              id: blockchainTokenEntityAdaptor.selectId(tokenArg)
            }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `unable to update token ${getAssetIdKey(tokenArg)}`,
            error
          )
        }
      },
      invalidatesTags: (_, __, tokenArg) => [
        { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
        { type: 'UserBlockchainTokens', id: getAssetIdKey(tokenArg) }
      ]
    }),
    updateUserAssetVisible: mutation<boolean, SetUserAssetVisiblePayloadType>({
      queryFn: async (
        { isVisible, token },
        { endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const {
            cache,
            data: { braveWalletService }
          } = baseQuery(undefined)

          cache.clearUserTokensRegistry()

          const { success } = await braveWalletService.setUserAssetVisible(
            token,
            isVisible
          )

          if (!success) {
            // token is probably not in the core-side assets list,
            // try adding it to the list
            const { success: addTokenSuccess } = await addUserToken({
              braveWalletService,
              cache,
              tokenArg: token
            })
            if (!addTokenSuccess) {
              throw new Error('Token could not be updated or added')
            }
          }

          return { data: success }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Could not user update asset visibility for token: ${
              getAssetIdKey(token) // token identifier
            }`,
            error
          )
        }
      },
      invalidatesTags: (res, err, arg) =>
        res
          ? [
              {
                type: 'UserBlockchainTokens',
                id: TOKEN_TAG_IDS.REGISTRY
              },
              {
                type: 'UserBlockchainTokens',
                id: getAssetIdKey(arg.token)
              }
            ]
          : ['UNKNOWN_ERROR']
    }),
    invalidateUserTokensRegistry: mutation<boolean, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          cache.clearUserTokensRegistry()
          return { data: true }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Could not invalidate user tokens registry',
            error
          )
        }
      },
      invalidatesTags: (res, err, arg) =>
        res
          ? [
              {
                type: 'UserBlockchainTokens',
                id: TOKEN_TAG_IDS.REGISTRY
              }
            ]
          : ['UNKNOWN_ERROR']
    })
  }
}

// Internals
async function addUserToken({
  braveWalletService,
  cache,
  tokenArg
}: {
  braveWalletService: BraveWallet.BraveWalletServiceRemote
  cache: BaseQueryCache
  tokenArg: BraveWallet.BlockchainToken
}) {
  cache.clearUserTokensRegistry()

  if (tokenArg.isErc721) {
    // Get NFTMetadata
    const metadata = await cache.getErc721Metadata({
      coin: tokenArg.coin,
      chainId: tokenArg.chainId,
      contractAddress: tokenArg.contractAddress,
      isErc721: tokenArg.isErc721,
      tokenId: tokenArg.tokenId,
      isNft: tokenArg.isNft
    })

    if (metadata?.image) {
      tokenArg.logo = metadata?.image || metadata?.image_url || tokenArg.logo
    }
  }

  return await braveWalletService.addUserAsset(tokenArg)
}
