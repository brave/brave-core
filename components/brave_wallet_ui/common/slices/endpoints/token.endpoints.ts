// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityId } from '@reduxjs/toolkit'

// types
import { BraveWallet } from '../../../constants/types'
import {
  TOKEN_TAG_IDS,
  WalletApiEndpointBuilderParams
} from '../api-base.slice'
import { type BaseQueryCache } from '../../async/base-query-cache'

// utils
import { handleEndpointError } from '../../../utils/api-utils'
import {
  addLogoToToken,
  getAssetIdKey,
  getDeletedTokenIds,
  getHiddenTokenIds
} from '../../../utils/asset-utils'
import { cacher } from '../../../utils/query-cache-utils'
import {
  BlockchainTokenEntityAdaptorState,
  blockchainTokenEntityAdaptor
} from '../entities/blockchain-token.entity'
import { LOCAL_STORAGE_KEYS } from '../../constants/local-storage-keys'
import { stripERC20TokenImageURL } from '../../../utils/string-utils'

export const tokenEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getTokensRegistry: query<BlockchainTokenEntityAdaptorState, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          return {
            data: await cache.getKnownTokensRegistry()
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
      queryFn: async (
        tokenArg,
        { dispatch, endpoint },
        extraOptions,
        baseQuery
      ) => {
        const {
          cache,
          data: { braveWalletService }
        } = baseQuery(undefined)

        const tokenIdentifier = blockchainTokenEntityAdaptor.selectId(tokenArg)

        try {
          cache.clearUserTokensRegistry()

          // update core user assets list
          const result = await addUserToken({
            braveWalletService,
            cache,
            tokenArg
          })

          if (!result.success) {
            throw new Error(`addUserToken failed`)
          }

          return {
            data: { id: tokenIdentifier }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Error adding user token: ${tokenIdentifier}`,
            error
          )
        }
      },
      invalidatesTags: (_, __, tokenArg) => [
        { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
        { type: 'UserBlockchainTokens', id: getAssetIdKey(tokenArg) },
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance',
        'PricesHistory'
      ]
    }),

    removeUserToken: mutation<
      boolean,
      /** tokenId */
      string
    >({
      queryFn: async (tokenId, { endpoint }, extraOptions, baseQuery) => {
        const {
          cache,
          data: { braveWalletService }
        } = baseQuery(undefined)

        try {
          const tokensRegistry = await cache.getUserTokensRegistry()
          const token = tokensRegistry.entities[tokenId]

          if (token?.contractAddress === '') {
            throw new Error('Removal of native assets not allowed')
          }

          // prevent showing the token in any list if it is auto-discovered
          const currentIds = getDeletedTokenIds()
          if (!currentIds.includes(tokenId)) {
            localStorage.setItem(
              LOCAL_STORAGE_KEYS.USER_DELETED_TOKEN_IDS,
              JSON.stringify(currentIds.concat(tokenId))
            )
          }

          // remove from user assets list
          if (token) {
            const deleteResult = await braveWalletService.removeUserAsset(token)
            if (!deleteResult.success) {
              throw new Error(
                'braveWalletService.removeUserAsset was unsuccessful'
              )
            }
          }

          cache.clearUserTokensRegistry()

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to remove user asset: ${tokenId}`,
            error
          )
        }
      },
      invalidatesTags: (_, __, tokenId) => [
        { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
        { type: 'UserBlockchainTokens', id: tokenId }
      ]
    }),

    updateUserToken: mutation<
      /** new asset id */
      string,
      {
        existingToken: BraveWallet.BlockchainToken
        updatedToken: BraveWallet.BlockchainToken
      }
    >({
      queryFn: async (
        { existingToken, updatedToken },
        { dispatch, endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const {
            cache,
            data: { braveWalletService }
          } = baseQuery(undefined)

          cache.clearUserTokensRegistry()

          // update core user assets list
          const deleteResult = await braveWalletService.removeUserAsset(
            existingToken
          )

          if (!deleteResult.success) {
            throw new Error('Unable to delete token')
          }

          const { success } = await addUserToken({
            braveWalletService,
            cache,
            tokenArg: updatedToken
          })

          if (!success) {
            throw new Error('Unable to add token')
          }

          return {
            data: getAssetIdKey(updatedToken)
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `unable to update token ${getAssetIdKey(updatedToken)}`,
            error
          )
        }
      },
      invalidatesTags: (_, __, { updatedToken }) => [
        { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
        { type: 'UserBlockchainTokens', id: getAssetIdKey(updatedToken) },
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance'
      ]
    }),

    updateUserAssetVisible: mutation<
      boolean,
      {
        token: BraveWallet.BlockchainToken
        isVisible: boolean
      }
    >({
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

          const tokenId = getAssetIdKey(token)

          // update local storage
          const currentHiddenIds = getHiddenTokenIds()
          if (isVisible) {
            localStorage.setItem(
              LOCAL_STORAGE_KEYS.USER_HIDDEN_TOKEN_IDS,
              JSON.stringify(currentHiddenIds.filter((id) => id !== tokenId))
            )
          }

          if (!isVisible && !currentHiddenIds.includes(tokenId)) {
            localStorage.setItem(
              LOCAL_STORAGE_KEYS.USER_HIDDEN_TOKEN_IDS,
              JSON.stringify(currentHiddenIds.concat(tokenId))
            )
          }

          // update asset in user assets list
          const { success } = await braveWalletService.setUserAssetVisible(
            token,
            isVisible
          )

          if (
            !success &&
            token.contractAddress !== '' // skip native assets to prevent errors in the core list
          ) {
            console.log(
              `update visibility for token (${
                tokenId //
              }) failed, attempting to add the token...`
            )
            // token is probably not in the core-side assets list,
            // try adding it to the list
            const { success: addTokenSuccess } = await addUserToken({
              braveWalletService,
              cache,
              tokenArg: { ...token, visible: isVisible }
            })
            if (!addTokenSuccess) {
              throw new Error('Token could not be updated or added')
            }
          }

          return { data: true }
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
              },
              'TokenBalances',
              'TokenBalancesForChainId',
              'AccountTokenCurrentBalance'
            ]
          : ['UNKNOWN_ERROR']
    }),

    getTokenInfo: query<
      BraveWallet.BlockchainToken | null,
      Pick<BraveWallet.BlockchainToken, 'chainId' | 'coin' | 'contractAddress'>
    >({
      queryFn: async (
        { coin, chainId, contractAddress },
        api,
        extraOptions,
        baseQuery
      ) => {
        try {
          if (coin !== BraveWallet.CoinType.ETH) {
            return {
              data: null
            }
          }

          const { jsonRpcService } = baseQuery(undefined).data
          const { token, error, errorMessage } =
            await jsonRpcService.getEthTokenInfo(contractAddress, chainId)

          if (error !== BraveWallet.ProviderError.kSuccess) {
            throw new Error(errorMessage)
          }

          return {
            data: token
          }
        } catch (err) {
          // Typically this means the token does not exist on the chain
          return handleEndpointError(
            api.endpoint,
            `Failed to get token info for: ${JSON.stringify(
              { coin, chainId, contractAddress },
              undefined,
              2
            )}`,
            err
          )
        }
      },
      providesTags: (result, err, args) =>
        err
          ? ['TokenInfo', 'UNKNOWN_ERROR']
          : [
              {
                type: 'TokenInfo',
                id: `${args.coin}-${args.chainId}-${args.contractAddress}`
              }
            ]
    }),
    getERC20Allowance: query<
      string, // allowance
      {
        contractAddress: string
        ownerAddress: string
        spenderAddress: string
        chainId: string
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const result = await api.jsonRpcService.getERC20TokenAllowance(
            arg.contractAddress,
            arg.ownerAddress,
            arg.spenderAddress,
            arg.chainId
          )

          if (result.error !== BraveWallet.ProviderError.kSuccess) {
            throw new Error(result.errorMessage)
          }

          return {
            data: result.allowance
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to get ERC20 allowance for: ${JSON.stringify(
              arg,
              undefined,
              2
            )}`,
            error
          )
        }
      }
    }),
    discoverAssets: mutation<true, void>({
      queryFn: async (
        _arg,
        { endpoint, dispatch },
        extraOptions,
        baseQuery
      ) => {
        try {
          const { data: api } = baseQuery(undefined)
          api.braveWalletService.discoverAssetsOnAllSupportedChains(true)
          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to start asset auto-discovery',
            error
          )
        }
      }
    }),

    // Token spam
    updateNftSpamStatus: mutation<
      boolean,
      {
        // Not using tokenId since spam NFTs are not
        // included in token registry by default
        token: BraveWallet.BlockchainToken
        isSpam: boolean
      }
    >({
      queryFn: async (arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const { braveWalletService } = api
          const { success } = await braveWalletService.setAssetSpamStatus(
            arg.token,
            arg.isSpam
          )

          // update user token

          cache.clearUserTokensRegistry()

          return {
            data: success
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error setting NFT spam status',
            error
          )
        }
      },
      invalidatesTags: (_res, err, arg) => {
        const tokenId = getAssetIdKey(arg.token)
        return err
          ? ['SimpleHashSpamNFTs', 'UserBlockchainTokens']
          : [
              { type: 'SimpleHashSpamNFTs', id: tokenId },
              { type: 'UserBlockchainTokens', id: tokenId }
            ]
      }
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
  if (tokenArg.isNft || tokenArg.isErc721 || tokenArg.isErc1155) {
    try {
      // Get NFTMetadata
      const metadata = await cache.getNftMetadata(tokenArg)

      if (metadata?.imageURL) {
        tokenArg = addLogoToToken(tokenArg, metadata.imageURL)
      }
    } catch (error) {
      console.log(error)
    }
  }

  const tokenIdentifier = getAssetIdKey(tokenArg)

  // token may have previously been deleted
  localStorage.setItem(
    LOCAL_STORAGE_KEYS.USER_DELETED_TOKEN_IDS,
    JSON.stringify(getDeletedTokenIds().filter((id) => id !== tokenIdentifier))
  )

  // token may have previously been hidden
  localStorage.setItem(
    LOCAL_STORAGE_KEYS.USER_HIDDEN_TOKEN_IDS,
    JSON.stringify(getHiddenTokenIds().filter((id) => id !== tokenIdentifier))
  )

  return await braveWalletService.addUserAsset({
    ...tokenArg,
    logo: stripERC20TokenImageURL(tokenArg.logo) || ''
  })
}
