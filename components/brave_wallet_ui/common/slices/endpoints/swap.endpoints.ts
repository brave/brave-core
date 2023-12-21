// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'

export const swapEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    generateBraveSwapFee: mutation<
      {
        response: BraveWallet.BraveSwapFeeResponse | null
        errorString: string
      },
      BraveWallet.BraveSwapFeeParams
    >({
      queryFn: async (params, { endpoint }, extraOptions, baseQuery) => {
        const { swapService } = baseQuery(undefined).data
        try {
          const result = await swapService.getBraveFee(params)
          return {
            data: result
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to fetch Brave Swap fee',
            error
          )
        }
      }
    }),

    generateSwapQuote: mutation<
      {
        response: BraveWallet.SwapQuoteUnion | null
        error: BraveWallet.SwapErrorUnion | null
        errorString: string
      },
      BraveWallet.SwapQuoteParams
    >({
      queryFn: async (params, { endpoint }, extraOptions, baseQuery) => {
        const { swapService } = baseQuery(undefined).data
        try {
          const result = await swapService.getQuote(params)
          return {
            data: result
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to generate Brave Swap quote',
            error
          )
        }
      }
    }),

    generateSwapTransaction: mutation<
      {
        response: BraveWallet.SwapTransactionUnion | null
        error: BraveWallet.SwapErrorUnion | null
        errorString: string
      },
      BraveWallet.SwapTransactionParamsUnion
    >({
      queryFn: async (params, { endpoint }, extraOptions, baseQuery) => {
        const { swapService } = baseQuery(undefined).data
        try {
          const result = await swapService.getTransaction(params)
          return {
            data: result
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to generate Brave Swap transaction',
            error
          )
        }
      }
    })
  }
}
