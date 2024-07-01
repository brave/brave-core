// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { filterLimit, mapLimit } from 'async'

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
  tokenNameToNftCollectionName
} from '../../../utils/asset-utils'
import { handleEndpointError } from '../../../utils/api-utils'
import {
  BaseQueryCache, //
  baseQueryFunction
} from '../../async/base-query-cache'
import {
  IPFS_PROTOCOL,
  stripERC20TokenImageURL
} from '../../../utils/string-utils'
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
    getNftMetadata: query<NFTMetadataReturnType, BraveWallet.BlockchainToken>({
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
    getNftPinningStatus: query<
      BraveWallet.TokenPinStatus | undefined,
      BraveWallet.BlockchainToken
    >({
      queryFn: async (arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { braveWalletPinService } = api
          const result = await braveWalletPinService.getTokenStatus(arg)

          if (result.error) {
            throw new Error(result.error.message)
          }

          if (result.status?.local) {
            return {
              data: result.status.local
            }
          }

          throw new Error('Local pinning status is null')
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error fetching NFT Pinning status',
            error
          )
        }
      },
      providesTags: (_result, err, arg) =>
        err
          ? ['NFTPinningStatus']
          : [{ type: 'NFTPinningStatus', id: getAssetIdKey(arg) }]
    }),
    getAutopinEnabled: query<boolean, void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { braveWalletAutoPinService } = baseQuery(undefined).data
          const result = await braveWalletAutoPinService.isAutoPinEnabled()

          return {
            data: result.enabled
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting autopin status',
            error
          )
        }
      },
      providesTags: ['AutoPinEnabled']
    }),
    setAutopinEnabled: mutation<
      boolean, // success
      boolean
    >({
      queryFn: async (arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { braveWalletAutoPinService } = baseQuery(undefined).data
          await braveWalletAutoPinService.setAutoPinEnabled(arg)

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error setting autopin status',
            error
          )
        }
      },
      invalidatesTags: ['AutoPinEnabled']
    }),
    getIPFSUrlFromGatewayLikeUrl: query<string | null, string>({
      queryFn: async (urlArg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          const ipfsUrl = await cache.getExtractedIPFSUrlFromGatewayLikeUrl(
            urlArg
          )
          return {
            data: ipfsUrl || null
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get IPFS URL from gateway-like URL',
            error
          )
        }
      }
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
    getNftsPinningStatus: query<
      /** `getAssetIdKey(asset)` */
      {
        [assetIdKey: string]: {
          code: BraveWallet.TokenPinStatusCode | undefined
          error: BraveWallet.PinError | undefined
        }
      },
      void
    >({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const { braveWalletPinService } = api
          const reg = await cache.getUserTokensRegistry()
          const visibleTokens = reg.visibleTokenIds
            .map((id) => reg.entities[id])
            .filter((token) => token?.isErc721 || token?.isNft)
          const results = await mapLimit(
            visibleTokens,
            10,
            async (token: BraveWallet.BlockchainToken) =>
              await braveWalletPinService.getTokenStatus(token)
          )
          const pinningStatus = {}
          visibleTokens.forEach((token, index) => {
            if (token) {
              const { status, error } = results[index]
              pinningStatus[getAssetIdKey(token)] = {
                code: status?.local?.code,
                error
              }
            }
          })
          return {
            data: pinningStatus
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting NFTs pinning status',
            error
          )
        }
      },
      providesTags: ['NFTSPinningStatus']
    }),
    getLocalIpfsNodeStatus: query<boolean, void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { braveWalletPinService } = baseQuery(undefined).data
          const isNodeRunning = await (
            await braveWalletPinService.isLocalNodeRunning()
          ).result

          return {
            data: isNodeRunning
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error fetching local node status',
            error
          )
        }
      },
      providesTags: ['LocalIPFSNodeStatus']
    }),
    getPinnableVisibleNftIds: query<string[], void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const {
            data: { walletHandler, braveWalletPinService },
            cache
          } = baseQuery(undefined)

          const {
            walletInfo: { isNftPinningFeatureEnabled }
          } = await walletHandler.getWalletInfo()

          if (!isNftPinningFeatureEnabled) {
            throw new Error('Pinning service is not enabled')
          }

          const userTokensRegistry = await cache.getUserTokensRegistry()
          const userVisibleNftIds =
            userTokensRegistry.nonFungibleVisibleTokenIds

          const pinnableNftIds = await filterLimit(
            userVisibleNftIds,
            10,
            async (id) => {
              const token = userTokensRegistry.entities[id]
              if (!token) {
                return false
              }

              // try to check metadata image
              let logo = token.logo
              try {
                logo =
                  (await cache.getNftMetadata(token)).imageURL || token.logo
              } catch (error) {
                console.log(`Failed to get NFT metadata for token ${id}`)
                console.error(error)
              }

              return isTokenSupportedForPinning({
                cache,
                braveWalletPinService,
                token: { ...token, logo }
              })
            }
          )

          return {
            data: pinnableNftIds
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get pinnable visible NFT ids',
            error
          )
        }
      },
      providesTags: ['PinnableNftIds']
    }),

    getIsImagePinnable: query<boolean, string>({
      queryFn: async (imageUrlArg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          const isPinnable = await cache.getIsImagePinnable(imageUrlArg)
          return {
            data: isPinnable
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to check if image URL (${imageUrlArg}) is pinnable`,
            error
          )
        }
      }
    })
  }
}

// Internals
export const isTokenSupportedForPinning = async ({
  cache,
  braveWalletPinService,
  token
}: {
  braveWalletPinService: BraveWallet.WalletPinServiceRemote
  cache: BaseQueryCache
  token: BraveWallet.BlockchainToken
}) => {
  const { result: isSupported } = await braveWalletPinService.isTokenSupported(
    token
  )

  if (!isSupported) {
    return false
  }

  const ipfsUrl = await cache.getExtractedIPFSUrlFromGatewayLikeUrl(
    stripERC20TokenImageURL(token.logo)
  )

  const isIpfsUrl = ipfsUrl && ipfsUrl.startsWith(IPFS_PROTOCOL)

  return isIpfsUrl
}
