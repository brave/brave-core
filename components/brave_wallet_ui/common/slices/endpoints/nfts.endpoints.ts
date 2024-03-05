// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'
import { EntityId } from '@reduxjs/toolkit'

// types
import {
  BraveWallet,
  ERC721Metadata,
  NFTMetadataReturnType
} from '../../../constants/types'
import {
  WalletApiEndpointBuilderParams,
  walletApiBase
} from '../api-base.slice'

// utils
import {
  GetBlockchainTokenIdArg,
  getAssetIdKey
} from '../../../utils/asset-utils'
import { cacher } from '../../../utils/query-cache-utils'
import {
  getAllNetworksList,
  getNetwork,
  handleEndpointError
} from '../../../utils/api-utils'

// entities
import { blockchainTokenEntityAdaptor } from '../entities/blockchain-token.entity'

export interface CommonNftMetadata {
  attributes?: any[]
  description?: string
  image?: string
  image_url?: string
  name?: string
}

export const nftsEndpoints = ({
  query,
  mutation
}: WalletApiEndpointBuilderParams) => {
  return {
    getERC721Metadata: query<
      {
        id: EntityId
        metadata?: ERC721Metadata
      },
      GetBlockchainTokenIdArg
    >({
      queryFn: async (tokenArg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { getErc721Metadata } = baseQuery(undefined).cache

          const metadata: ERC721Metadata = await getErc721Metadata(tokenArg)

          return {
            data: {
              id: blockchainTokenEntityAdaptor.selectId(tokenArg),
              metadata
            }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error fetching ERC-721 metadata',
            error
          )
        }
      },
      providesTags: cacher.cacheByBlockchainTokenArg('ERC721Metadata')
    }),
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
          const { data: api, cache } = baseQuery(undefined)
          const { jsonRpcService } = api
          const result =
            arg.coin === BraveWallet.CoinType.ETH
              ? await jsonRpcService.getERC721Metadata(
                  arg.contractAddress,
                  arg.tokenId,
                  arg.chainId
                )
              : arg.coin === BraveWallet.CoinType.SOL
              ? await jsonRpcService.getSolTokenMetadata(
                  arg.chainId,
                  arg.contractAddress
                )
              : undefined

          if (result?.error) throw new Error(result.errorMessage)

          const response = result?.response
            ? (JSON.parse(result.response) as CommonNftMetadata)
            : undefined

          const responseImageUrl = response?.image || response?.image_url
          const imageURL = responseImageUrl?.startsWith('data:image/')
            ? responseImageUrl
            : await cache.getIpfsGatewayTranslatedNftUrl(responseImageUrl || '')

          const attributes = Array.isArray(response?.attributes)
            ? response?.attributes.map(
                (attr: { trait_type: string; value: string }) => ({
                  traitType: attr.trait_type,
                  value: attr.value
                })
              )
            : []
          const tokenNetwork = await getNetwork(api, arg)
          const nftMetadata: NFTMetadataReturnType = {
            metadataUrl: result?.tokenUrl || '',
            chainName: tokenNetwork?.chainName || '',
            tokenType:
              arg.coin === BraveWallet.CoinType.ETH
                ? 'ERC721'
                : arg.coin === BraveWallet.CoinType.SOL
                ? 'SPL'
                : '',
            tokenID: arg.tokenId,
            imageURL: imageURL || undefined,
            imageMimeType: 'image/*',
            floorFiatPrice: '',
            floorCryptoPrice: '',
            contractInformation: {
              address: arg.contractAddress,
              name: response?.name || '???',
              description: response?.description || '???',
              website: '',
              facebook: '',
              logo: '',
              twitter: ''
            },
            attributes
          }

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
          ? ['ERC721Metadata']
          : [{ type: 'ERC721Metadata', id: getAssetIdKey(arg) }]
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
    getSimpleHashSpamNfts: query<BraveWallet.BlockchainToken[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const { braveWalletService } = api
          const chainIds = (await getAllNetworksList(api)).map(
            (network) => network.chainId
          )

          const { accounts } = await cache.getAllAccounts()
          const spamNfts = (
            await mapLimit(
              accounts,
              10,
              async (account: BraveWallet.AccountInfo) => {
                let currentCursor: string | null = null
                const accountSpamNfts = []

                do {
                  const {
                    tokens,
                    cursor
                  }: {
                    tokens: BraveWallet.BlockchainToken[]
                    cursor: string | null
                  } = await braveWalletService.getSimpleHashSpamNFTs(
                    account.address,
                    chainIds,
                    account.accountId.coin,
                    currentCursor
                  )

                  accountSpamNfts.push(...tokens)
                  currentCursor = cursor
                } while (currentCursor)

                return accountSpamNfts
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
    updateNftsPinningStatus: mutation<
      boolean,
      {
        token: BraveWallet.BlockchainToken
        status: BraveWallet.TokenPinStatus
      }
    >({
      queryFn: () => ({ data: true }),
      async onQueryStarted({ token, status }, { dispatch, queryFulfilled }) {
        const patchResult = dispatch(
          walletApiBaseUtils.updateQueryData(
            'getNftsPinningStatus',
            undefined,
            (nftsPinningStatus: {}) => {
              Object.assign(nftsPinningStatus, {
                ...nftsPinningStatus,
                [getAssetIdKey(token)]: {
                  code: status?.code,
                  error: status?.error
                }
              })
            }
          )
        )
        try {
          await queryFulfilled
        } catch {
          patchResult.undo()
        }
      }
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
    })
  }
}

const walletApiBaseUtils = walletApiBase.injectEndpoints({
  endpoints: nftsEndpoints,
  overrideExisting: false
}).util
