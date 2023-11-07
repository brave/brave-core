// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'

export type GetCoinMarketArgs = {
  vsAsset: string
  limit: number
}

export const coinMarketEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getCoinMarket: query<BraveWallet.CoinMarket[], GetCoinMarketArgs>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const {
            data: { assetRatioService }
          } = baseQuery(undefined)
          const result = await assetRatioService.getCoinMarkets(
            arg.vsAsset,
            arg.limit
          )
          return {
            data: result.values.map((coin) => {
              coin.image = coin.image.replace(
                'https://assets.coingecko.com',
                ' https://assets.cgproxy.brave.com'
              )
              return coin
            })
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to fetch coin market data',
            error
          )
        }
      }
    })
  }
}
