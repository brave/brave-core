// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'

// types
import { BraveWallet, NFTMetadataReturnType } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { getAssetIdKey } from '../../../utils/asset-utils'
import { handleEndpointError } from '../../../utils/api-utils'

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
    })
  }
}
