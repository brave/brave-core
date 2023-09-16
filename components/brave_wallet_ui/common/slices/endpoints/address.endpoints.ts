// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'

export const addressEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
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

    getEnsAddress: query<
      { address: string; requireOffchainConsent: boolean },
      string
    >({
      queryFn: async (addressArg, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { address, errorMessage, requireOffchainConsent } =
            await api.jsonRpcService.ensGetEthAddr(addressArg)

          if (errorMessage) {
            throw new Error(errorMessage)
          }

          return {
            data: {
              address,
              requireOffchainConsent
            }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to get ENS Address for: ${addressArg}`,
            error
          )
        }
      }
    }),

    getSnsAddress: query<string, string>({
      queryFn: async (addressArg, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { address, errorMessage } =
            await api.jsonRpcService.snsGetSolAddr(addressArg)

          if (errorMessage) {
            throw new Error(errorMessage)
          }

          return {
            data: address
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to get SNS Address for: ${addressArg}`,
            error
          )
        }
      }
    }),

    getUDAddress: query<
      string,
      { address: string; token: BraveWallet.BlockchainToken | null }
    >({
      queryFn: async (arg, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { address, errorMessage } =
            await api.jsonRpcService.unstoppableDomainsGetWalletAddr(
              arg.address,
              arg.token
            )

          if (errorMessage) {
            throw new Error(errorMessage)
          }

          return {
            data: address
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to get Unstoppable-Domains Address for: address: ${
              arg.address //
            },
              token: ${
                JSON.stringify(arg.token) //
              }`,
            error
          )
        }
      }
    }),

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
    })
  }
}
