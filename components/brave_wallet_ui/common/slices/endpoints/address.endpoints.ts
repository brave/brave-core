// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'
import { endsWithAny } from '../../../utils/string-utils'
import { allSupportedExtensions, supportedENSExtensions, supportedSNSExtensions, supportedUDExtensions } from '../../constants/domain-extensions'

export const addressEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    enableEnsOffchainLookup: mutation<boolean, void>({
      queryFn: async (_arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          api.jsonRpcService.setEnsOffchainLookupResolveMethod(
            BraveWallet.ResolveMethod.kEnabled
          )
          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to enable Ens Off-chain Lookup',
            error
          )
        }
      },
      invalidatesTags: ['EnsOffchainLookupEnabled']
    }),

    getIsBase58EncodedSolPubkey: query<boolean, string>({
      queryFn: async (pubKeyArg, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { result } =
            await api.braveWalletService.isBase58EncodedSolanaPubkey(pubKeyArg)

          return {
            data: result
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to check Base58 encoding for pubkey: ${pubKeyArg}`,
            error
          )
        }
      }
    }),
    
    getEthAddressChecksum: query<string, string>({
      queryFn: async (addressArg, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { checksumAddress } = await api.keyringService.getChecksumEthAddress(addressArg)

          return {
            data: checksumAddress
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to check Base58 encoding for pubkey: ${addressArg}`,
            error
          )
        }
      }
    }),

    getAddressFromNameServiceUrl: query<
    { address: string; requireOffchainConsent: boolean },
    { url: string; tokenId: string | null }
  >({
      queryFn: async (arg, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)

          // Ens
          if (endsWithAny(supportedENSExtensions, arg.url)) {
            const { address, errorMessage, requireOffchainConsent } =
              await api.jsonRpcService.ensGetEthAddr(arg.url)

            if (errorMessage) {
              throw new Error(errorMessage)
            }
  
            return {
              data: {
                address,
                requireOffchainConsent
              }
            }
          }
          
          // Sns
          if (endsWithAny(supportedSNSExtensions, arg.url)) {
            const { address, errorMessage } =
              await api.jsonRpcService.snsGetSolAddr(arg.url)

            if (errorMessage) {
              throw new Error(errorMessage)
            }

            return {
              data: {
                address,
                requireOffchainConsent: false
              }
            }
          }
          
          // Unstoppable-Domains
          if (endsWithAny(supportedUDExtensions, arg.url)) {
            const token = arg.tokenId
              ? (await cache.getUserTokensRegistry()).entities[arg.tokenId] ||
                null
              : null

            const { address, errorMessage } =
              await api.jsonRpcService.unstoppableDomainsGetWalletAddr(
                arg.url,
                token
              )

            if (errorMessage) {
              throw new Error(errorMessage)
            }

            return {
              data: {
                address,
                requireOffchainConsent: false
              }
            }
          }

          throw new Error(
            `${
              arg.url
            } does not end in a valid extension (${allSupportedExtensions.join(
              ', '
            )})`
          )
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to lookup Address for domain URL: url: ${
              arg.url //
            },
              tokenId: ${arg.tokenId}`,
            error
          )
        }
      }
    }),
  }
}