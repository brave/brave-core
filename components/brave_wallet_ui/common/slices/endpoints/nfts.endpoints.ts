// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityId } from '@reduxjs/toolkit'

// types
import {
  BraveWallet,
  CoinType,
  ERC721Metadata,
  NFTMetadataReturnType
} from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import {
  GetBlockchainTokenIdArg,
  getAssetIdKey
} from '../../../utils/asset-utils'
import { cacher } from '../../../utils/query-cache-utils'

// entities
import { blockchainTokenEntityAdaptor } from '../entities/blockchain-token.entity'

// api
import { getNetwork } from '../api.slice'

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
      queryFn: async (tokenArg, _api, _extraOptions, baseQuery) => {
        try {
          if (!tokenArg.isErc721) {
            throw new Error(
              'Cannot fetch erc-721 metadata for non erc-721 token'
            )
          }

          const { jsonRpcService } = baseQuery(undefined).data

          const result = await jsonRpcService.getERC721Metadata(
            tokenArg.contractAddress,
            tokenArg.tokenId,
            tokenArg.chainId
          )

          if (result.error || result.errorMessage) {
            throw new Error(result.errorMessage)
          }

          const metadata: ERC721Metadata = JSON.parse(result.response)
          return {
            data: {
              id: blockchainTokenEntityAdaptor.selectId(tokenArg),
              metadata
            }
          }
        } catch (error) {
          console.error('Error fetching ERC-721 metadata', error)
          return {
            error: `Error fetching ERC-721 metadata: ${error}`
          }
        }
      },
      providesTags: cacher.cacheByBlockchainTokenArg('ERC721Metadata')
    }),
    getNftDiscoveryEnabledStatus: query<boolean, void>({
      queryFn: async (_arg, _api, _extraOptions, baseQuery) => {
        try {
          const { braveWalletService } = baseQuery(undefined).data
          const result = await braveWalletService.getNftDiscoveryEnabled()
          return {
            data: result.enabled
          }
        } catch (error) {
          console.error('Error getting NFT discovery status: ', error)
          return { error: 'Failed to fetch NFT auto-discovery status' }
        }
      },
      providesTags: ['NftDiscoveryEnabledStatus']
    }),
    setNftDiscoveryEnabled: mutation<
      boolean, // success
      boolean
    >({
      queryFn: async (arg, _api, _extraOptions, baseQuery) => {
        try {
          const { braveWalletService } = baseQuery(undefined).data
          await braveWalletService.setNftDiscoveryEnabled(arg)

          return {
            data: true
          }
        } catch (error) {
          console.error('Error setting NFT discovery status: ', error)
          return {
            error: 'Failed to set NFT auto-discovery status'
          }
        }
      },
      invalidatesTags: ['NftDiscoveryEnabledStatus']
    }),
    getNftMetadata: query<NFTMetadataReturnType, BraveWallet.BlockchainToken>({
      queryFn: async (arg, _api, _extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const { jsonRpcService } = api
          const result =
            arg.coin === CoinType.ETH
              ? await jsonRpcService.getERC721Metadata(
                  arg.contractAddress,
                  arg.tokenId,
                  arg.chainId
                )
              : arg.coin === CoinType.SOL
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
              arg.coin === CoinType.ETH
                ? 'ERC721'
                : arg.coin === CoinType.SOL
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
          const message =
            'Error fetching NFT metadata: ' + error?.message ||
            JSON.stringify(error)
          console.error(message)
          return {
            error: message
          }
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
      queryFn: async (arg, _api, _extraOptions, baseQuery) => {
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
          const message =
            'Error fetching NFT Pinning status: ' + error?.message ||
            JSON.stringify(error)
          console.error(message)
          return {
            error: message
          }
        }
      },
      providesTags: (_result, err, arg) =>
        err
          ? ['NFTPinningStatus']
          : [{ type: 'NFTPinningStatus', id: getAssetIdKey(arg) }]
    }),
    getAutopinEnabled: query<boolean, void>({
      queryFn: async (_arg, _api, _extraOptions, baseQuery) => {
        try {
          const { braveWalletAutoPinService } = baseQuery(undefined).data
          const result = await braveWalletAutoPinService.isAutoPinEnabled()

          return {
            data: result.enabled
          }
        } catch (error) {
          console.error('Error getting autopin status: ', error)
          const message =
            'Error getting auto-pin status: ' + error?.message ||
            JSON.stringify(error)
          return {
            error: message
          }
        }
      },
      providesTags: ['AutoPinEnabled']
    }),
    getIPFSUrlFromGatewayLikeUrl: query<string | null, string>({
      queryFn: async (urlArg, store, extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          const ipfsUrl = await cache.getExtractedIPFSUrlFromGatewayLikeUrl(
            urlArg
          )
          return {
            data: ipfsUrl || null
          }
        } catch (error) {
          const message = 'Failed to get IPFS URL from gateway-like URL'
          console.log(`${message}: %s`, urlArg.trim(), error)
          console.error(error)
          return { error: message }
        }
      }
    }),
    getIpfsGatewayTranslatedNftUrl: query<string | null, string>({
      queryFn: async (urlArg, store, extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          const translatedUrl = await cache.getIpfsGatewayTranslatedNftUrl(
            urlArg || ''
          )

          return {
            data: translatedUrl || urlArg.trim()
          }
        } catch (error) {
          const message = 'Failed to translate NFT IPFS gateway URL'
          console.log(`${message}: %s`, urlArg.trim(), error)
          console.error(error)
          return { error: message }
        }
      }
    })
  }
}
