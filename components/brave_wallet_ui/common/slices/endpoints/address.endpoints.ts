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
import {
  allSupportedExtensions,
  supportedENSExtensions,
  supportedSNSExtensions,
  supportedUDExtensions
} from '../../constants/domain-extensions'

interface GetFVMAddressArg {
  coin: BraveWallet.CoinType | undefined
  isMainNet: boolean
  addresses: string[]
}

type GetFVMAddressResult = Map<string, { address: string; fvmAddress: string }>

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
      invalidatesTags: ['NameServiceAddress', 'EnsOffchainLookupEnabled']
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
          const { checksumAddress } =
            await api.keyringService.getChecksumEthAddress(addressArg)

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

    validateUnifiedAddress: query<
      BraveWallet.ZCashAddressValidationResult,
      {
        address: string,
        testnet: boolean
      }
    >({
      queryFn: async (arg, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { result } =
            await api.zcashWalletService.validateZCashAddress(
                arg.address, arg.testnet)
          return {
            data: result
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to validate Zcash address: ${arg.address}`,
            error
          )
        }
      }
    }),

    getAddressFromNameServiceUrl: query<
      { address: string; requireOffchainConsent: boolean },
      {
        /**
         * Name service URLs are case-insensitive, but this arg should always be
         * lowercase to prevent refetching of resolutions for casing changes
         */
        url: string
        /**
         * Only used by Unstoppable Domains
         */
        tokenId: string | null
      }
    >({
      queryFn: async (arg, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)

          // https://github.com/brave/brave-browser/issues/34796
          // name service URLs are case-insensitive, but backend currently
          // fails to resolve addresses for URLS containing capital letters
          const lowercaseURL = arg.url.toLowerCase()

          // Ens
          if (endsWithAny(supportedENSExtensions, lowercaseURL)) {
            const { address, errorMessage, requireOffchainConsent } =
              await api.jsonRpcService.ensGetEthAddr(lowercaseURL)

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
          if (endsWithAny(supportedSNSExtensions, lowercaseURL)) {
            const { address, errorMessage } =
              await api.jsonRpcService.snsGetSolAddr(lowercaseURL)

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
          if (endsWithAny(supportedUDExtensions, lowercaseURL)) {
            const token = arg.tokenId
              ? (await cache.getUserTokensRegistry()).entities[arg.tokenId] ||
                null
              : null

            const { address, errorMessage } =
              await api.jsonRpcService.unstoppableDomainsGetWalletAddr(
                lowercaseURL,
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
      },
      providesTags: (res, err, arg) => [
        err
          ? 'UNKNOWN_ERROR'
          : {
              type: 'NameServiceAddress',
              id: [arg.url.toLowerCase(), arg.tokenId]
                .filter((arg) => arg !== null)
                .join('-')
            }
      ]
    }),

    getFVMAddress: query<GetFVMAddressResult, GetFVMAddressArg>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        if (arg.coin !== BraveWallet.CoinType.FIL) {
          // invalid coin type
          return { data: {} }
        }
        try {
          const { braveWalletService } = baseQuery(undefined).data
          const convertResult = (
            await braveWalletService.convertFEVMToFVMAddress(
              arg.isMainNet,
              arg.addresses
            )
          ).result
          return {
            data: convertResult
          }
        } catch (error) {
          return handleEndpointError(endpoint, 'Unable to getFVMAddress', error)
        }
      }
    }),

    generateReceiveAddress: mutation<string, BraveWallet.AccountId>({
      queryFn: async (accountId, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { braveWalletService } = baseQuery(undefined).data
          const { address, errorMessage } =
            await braveWalletService.generateReceiveAddress(accountId)

          if (!address || errorMessage) {
            throw new Error(errorMessage ?? 'Unknown error')
          }

          return {
            data: address
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable generate receive address for account: ' +
              accountId.uniqueKey,
            error
          )
        }
      }
    })
  }
}
