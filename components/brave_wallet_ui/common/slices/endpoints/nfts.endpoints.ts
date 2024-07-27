// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'

// types
import {
  AssetIdsByCollectionNameRegistry,
  BraveWallet,
  NFTMetadataReturnType
} from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import {
  getAssetIdKey,
  GetBlockchainTokenIdArg,
  tokenNameToNftCollectionName
} from '../../../utils/asset-utils'
import { handleEndpointError } from '../../../utils/api-utils'
import { baseQueryFunction } from '../../async/base-query-cache'
import {
  getPersistedNftCollectionNamesRegistry,
  setPersistedNftCollectionNamesRegistry
} from '../../../utils/local-storage-utils'

export const nftsEndpoints = ({
  query,
  mutation
}: WalletApiEndpointBuilderParams) => {
  return {
    getNftDiscoveryEnabledStatus: query<boolean, void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { braveWalletService } = baseQuery(undefined).data
          const result = await braveWalletService.getNftDiscoveryEnabled()
          return {
            data: result.enabled
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting NFT discovery status: ',
            error
          )
        }
      },
      providesTags: ['NftDiscoveryEnabledStatus']
    }),
    setNftDiscoveryEnabled: mutation<
      boolean, // success
      boolean
    >({
      queryFn: async (arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { braveWalletService } = baseQuery(undefined).data
          await braveWalletService.setNftDiscoveryEnabled(arg)

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error setting NFT discovery status: ',
            error
          )
        }
      },
      invalidatesTags: ['NftDiscoveryEnabledStatus']
    }),
    getNftMetadata: query<NFTMetadataReturnType, GetBlockchainTokenIdArg>({
      queryFn: async (arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)

          const nftMetadata = await cache.getNftMetadata(arg)

          return {
            data: nftMetadata
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error fetching NFT metadata',
            error
          )
        }
      },
      providesTags: (_result, err, arg) =>
        err
          ? ['NftMetadata']
          : [{ type: 'NftMetadata', id: getAssetIdKey(arg) }]
    }),
    getIpfsGatewayTranslatedNftUrl: query<string | null, string>({
      queryFn: async (urlArg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          const translatedUrl = await cache.getIpfsGatewayTranslatedNftUrl(
            urlArg || ''
          )

          return {
            data: translatedUrl || urlArg.trim()
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to translate NFT IPFS gateway URL',
            error
          )
        }
      }
    }),
    /**
     * Fetches a registry of NFT collection names by collection asset id.
     * Uses local storage to load the initial data and
     * then fetches the latest data from the token contract or NFT metadata
     */
    getNftAssetIdsByCollectionRegistry: query<
      /** collection names by collection asset id */
      { registry: AssetIdsByCollectionNameRegistry; isStreaming: boolean },
      /** asset id keys */
      BraveWallet.BlockchainToken[]
    >({
      queryFn(_arg, api, _extraOptions, baseQuery) {
        return {
          data: {
            isStreaming: true,
            registry: getPersistedNftCollectionNamesRegistry()
          }
        }
      },
      async onCacheEntryAdded(
        arg,
        { updateCachedData, cacheDataLoaded, cacheEntryRemoved }
      ) {
        try {
          // wait for the initial query to resolve before proceeding
          const {
            data: { registry: persistedRegistry }
          } = await cacheDataLoaded

          try {
            const {
              cache,
              data: { jsonRpcService }
            } = baseQueryFunction()

            const registry: AssetIdsByCollectionNameRegistry = {}

            // prevent looking up token info multiple times
            const uniqueTokenChainIdAndContractAddresses: string[] = []
            const uniqueTokens: BraveWallet.BlockchainToken[] = arg.filter(
              (t) => {
                const key = `${t.chainId}_${t.contractAddress}`
                if (uniqueTokenChainIdAndContractAddresses.includes(key)) {
                  return false
                }
                uniqueTokenChainIdAndContractAddresses.push(key)
                return true
              }
            )

            for (const token of uniqueTokens) {
              let collectionName = ''

              // EVM lookup using contract info
              if (token.coin === BraveWallet.CoinType.ETH) {
                const {
                  token: tokenInfo,
                  error,
                  errorMessage
                } = await jsonRpcService.getEthTokenInfo(
                  token.contractAddress,
                  token.chainId
                )

                if (error !== BraveWallet.ProviderError.kSuccess) {
                  throw new Error(errorMessage)
                }

                collectionName = tokenInfo?.name || ''
              }

              // use NFT metadata to find collection name if it could not be
              // found in the EVM token info
              let nftMetadata = null
              if (!collectionName) {
                try {
                  nftMetadata = await cache.getNftMetadata(token)
                  collectionName =
                    nftMetadata?.collection?.name ||
                    // guess the collection name
                    // if no contract or metadata could be found
                    tokenNameToNftCollectionName(token)
                } catch (error) {
                  handleEndpointError(
                    'getNftCollectionNameRegistry -> cache.getNftMetadata',
                    `Error fetching NFT metadata for token: ${
                      token.name
                    } (${getAssetIdKey(token)})`,
                    error
                  )
                }
              }

              // initialize the collection assets list in the registry if needed
              if (!registry[collectionName]) {
                registry[collectionName] = []
              }

              // add the asset id to the collection assets list
              registry[collectionName].push(
                // remove the token id to reduce the sie of the registry
                getAssetIdKey({ ...token, tokenId: '' })
              )
            }

            updateCachedData((draft) => {
              // rules of `immer` apply here:
              // https://redux-toolkit.js.org/usage/immer-reducers#immer-usage-patterns
              draft.isStreaming = false
              draft.registry = registry
            })

            // update local-storage with the latest data
            setPersistedNftCollectionNamesRegistry({
              ...persistedRegistry,
              ...registry
            })
          } catch (error) {
            handleEndpointError(
              'getNftCollectionNameRegistry.onCacheEntryAdded',
              'Error fetching NFT collection name registry',
              error
            )
          }
        } catch {
          // cacheDataLoaded` will throw if
          // `cacheEntryRemoved` resolves before `cacheDataLoaded`
        }
        // cacheEntryRemoved will resolve when the cache subscription is no
        // longer active
      },
      providesTags: (_result, err, arg) => [
        { type: 'NftMetadata', id: 'COLLECTION_NAMES_REGISTRY' }
      ]
    }),

    /** will get spam for all accounts if accounts arg is not provided */
    getSimpleHashSpamNfts: query<
      BraveWallet.BlockchainToken[],
      void | undefined | { accounts: BraveWallet.AccountInfo[] }
    >({
      queryFn: async (arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)

          const lookupAccounts =
            arg?.accounts ?? (await cache.getAllAccounts()).accounts

          const spamNfts = (
            await mapLimit(
              lookupAccounts,
              10,
              async (account: BraveWallet.AccountInfo) => {
                return await cache.getSpamNftsForAccountId(account.accountId)
              }
            )
          ).flat(1)

          return {
            data: spamNfts
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to fetch spam NFTs',
            error
          )
        }
      },
      providesTags: ['SimpleHashSpamNFTs']
    }),

    getNftOwner: query<
      string, // owner address
      { contract: string; tokenId: string; chainId: string }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { jsonRpcService } = api
          const { errorMessage, ownerAddress } =
            await jsonRpcService.getERC721OwnerOf(
              arg.contract,
              arg.tokenId,
              arg.chainId
            )
          if (errorMessage) {
            throw new Error(errorMessage)
          }
          return {
            data: ownerAddress
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to fetch owner address for NFT(${
              arg.contract //
            }-${arg.tokenId}) on chain(${arg.chainId})`,
            error
          )
        }
      }
    })
  }
}
