// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import {
  BraveWallet
} from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

export const coingeckoEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getCoingeckoId: query<
      string | undefined,
      Pick<BraveWallet.BlockchainToken, 'chainId' | 'contractAddress'>
    >({
      queryFn: async (
        { chainId, contractAddress },
        api,
        extraOptions,
        baseQuery
      ) => {
        // Ignore invalid EVM and Solana addresses.
        //
        // EVM => 0x + 40 hex characters
        // Solana => 32-44 base58 characters
        if (
          !/0x[a-fA-F0-9]{40}/.test(contractAddress) &&
          !/[1-9A-HJ-NP-Za-km-z]{32,44}/.test(contractAddress)
        ) {
          return {
            data: undefined
          }
        }

        const { blockchainRegistry } = baseQuery(undefined).data
        const { coingeckoId } = await blockchainRegistry.getCoingeckoId(
          chainId,
          contractAddress
        )

        return {
          data: coingeckoId ?? undefined
        }
      },
      providesTags: (res, err, { chainId, contractAddress }) =>
        err
          ? ['CoingeckoId', 'UNKNOWN_ERROR']
          : [{ type: 'CoingeckoId', id: `${chainId}-${contractAddress}` }]
    })
  }
}
