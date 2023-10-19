// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// Utils
import {
  isValidEVMAddress,
  isValidSolanaAddress
} from '../../../utils/address-utils'

export const coingeckoEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getCoingeckoId: query<
      string | null,
      Pick<BraveWallet.BlockchainToken, 'chainId' | 'contractAddress'>
    >({
      queryFn: async (
        { chainId, contractAddress },
        api,
        extraOptions,
        baseQuery
      ) => {
        try {
          // Ignore invalid EVM and Solana addresses.
          //
          // EVM => 0x + 40 hex characters
          // Solana => 32-44 base58 characters
          if (
            !isValidEVMAddress(contractAddress) &&
            !isValidSolanaAddress(contractAddress)
          ) {
            return {
              data: null
            }
          }

          const { blockchainRegistry } = baseQuery(undefined).data
          const { coingeckoId } = await blockchainRegistry.getCoingeckoId(
            chainId,
            contractAddress
          )

          return {
            data: coingeckoId
          }
        } catch (err) {
          console.error(err)
          return {
            error: 'Unable to query coingeckoId'
          }
        }
      },
      providesTags: (res, err, { chainId, contractAddress }) =>
        err
          ? ['CoingeckoId', 'UNKNOWN_ERROR']
          : [{ type: 'CoingeckoId', id: `${chainId}-${contractAddress}` }]
    })
  }
}
