// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { mapLimit } from 'async'

// Types
import {
  BraveWallet,
  TokenPriceHistory,
  SpotPriceRegistry
} from '../../../constants/types'
import {
  TokenBalancesRegistry,
  ChainBalances
} from '../entities/token-balance.entity'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// Constants
import { maxConcurrentPriceRequests, maxBatchSizePrice } from '../constants'
import { SKIP_PRICE_LOOKUP_COINGECKO_ID } from '../../constants/magics'

// Utils
import { makeSerializableTimeDelta } from '../../../utils/model-serialization-utils'
import { getPriceIdForToken } from '../../../utils/pricing-utils'
import Amount from '../../../utils/amount'
import { findTokenByAssetId } from '../../../utils/asset-utils'

interface GetTokenSpotPricesArg {
  ids: string[]
  timeframe?: BraveWallet.AssetPriceTimeframe
  toCurrency: string
}

interface GetPriceHistoryArg {
  /**
   * Token parameter from getPriceIdForToken(asset)
   */
  tokenParam: string
  /**
   * Default currency
   */
  vsAsset: string
  /**
   * Timeframe for which price history will be fetched
   */
  timeFrame: number
}

interface GetPricesHistoryArg {
  tokens: Array<
    Pick<
      BraveWallet.BlockchainToken,
      | 'contractAddress'
      | 'chainId'
      | 'coin'
      | 'decimals'
      | 'symbol'
      | 'coingeckoId'
      | 'tokenId'
      | 'isShielded'
    >
  >

  /**
   * Default currency
   */
  vsAsset: string

  /**
   * Timeframe for which price history will be fetched
   */
  timeframe: BraveWallet.AssetPriceTimeframe

  /**
   * TokenBalancesRegistry to skip looking up prices for tokens that are not
   * in the registry or have zero balance.
   */
  tokenBalancesRegistry: TokenBalancesRegistry
}

export const pricingEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getTokenSpotPrices: query<SpotPriceRegistry, GetTokenSpotPricesArg>({
      queryFn: async (
        { ids, timeframe, toCurrency },
        { dispatch },
        extraOptions,
        baseQuery
      ) => {
        try {
          const {
            data: { assetRatioService }
          } = baseQuery(undefined)

          if (!ids.length) {
            throw new Error('no token ids provided for price lookup')
          }

          // dedupe ids to prevent duplicate price requests
          const uniqueIds = [...new Set(ids)]
            // skip flagged coins such as testnet coins
            .filter((id) => id !== SKIP_PRICE_LOOKUP_COINGECKO_ID)

          const chunkedParams = []
          for (let i = 0; i < uniqueIds.length; i += maxBatchSizePrice) {
            chunkedParams.push(uniqueIds.slice(i, i + maxBatchSizePrice))
          }

          // Use maxConcurrentPriceRequests concurrent HTTP requests to
          // fetch prices, in batch of maxBatchSizePrice.
          const results = await mapLimit(
            chunkedParams,
            maxConcurrentPriceRequests,
            async function (params: string[]) {
              const { success, values } = await assetRatioService.getPrice(
                params,
                [toCurrency],
                timeframe ?? BraveWallet.AssetPriceTimeframe.Live
              )

              if (success && values) {
                return values
              }

              console.log('Unable to fetch prices for batch:', params)
              const fallbackResults = await mapLimit(
                params,
                maxConcurrentPriceRequests,
                async function (param: string) {
                  const { success, values } = await assetRatioService.getPrice(
                    [param],
                    [toCurrency],
                    timeframe ?? BraveWallet.AssetPriceTimeframe.Live
                  )

                  if (success) {
                    return values
                  }

                  console.log('Unable to fetch price using fallback:', param)

                  return []
                }
              )

              return fallbackResults.flat()
            }
          )

          const registry: SpotPriceRegistry = results
            .flat()
            .reduce<SpotPriceRegistry>((acc, assetPrice) => {
              acc[assetPrice.fromAsset.toLowerCase()] = assetPrice
              return acc
            }, {})

          // add skipped value
          registry[SKIP_PRICE_LOOKUP_COINGECKO_ID] = {
            assetTimeframeChange: (
              timeframe ?? BraveWallet.AssetPriceTimeframe.Live
            ).toString(),
            fromAsset: SKIP_PRICE_LOOKUP_COINGECKO_ID,
            price: '0',
            toAsset: toCurrency
          }

          return {
            data: registry
          }
        } catch (error) {
          const msg = `Unable to fetch prices`
          console.error(`${msg}: ${error}`)
          return {
            error: msg
          }
        }
      },
      providesTags: (result, error, { ids, timeframe }) =>
        ids.map((id) => ({
          type: 'TokenSpotPrices',
          id: `${id}-${timeframe ?? BraveWallet.AssetPriceTimeframe.Live}`
        }))
    }),

    getPriceHistory: query<TokenPriceHistory[], GetPriceHistoryArg>({
      queryFn: async (
        { tokenParam, vsAsset, timeFrame },
        _api,
        _extraOptions,
        baseQuery
      ) => {
        try {
          const {
            data: { assetRatioService }
          } = baseQuery(undefined)
          const { success, values } = await assetRatioService.getPriceHistory(
            tokenParam,
            vsAsset,
            timeFrame
          )

          if (success && values) {
            return {
              data: values.map((value) => ({
                date: makeSerializableTimeDelta(value.date),
                close: Number(value.price)
              }))
            }
          }

          throw new Error(
            `Unable to fetch price history for token: ${tokenParam}`
          )
        } catch (error) {
          const message =
            'Error getting price history: ' + error?.message ||
            JSON.stringify(error)
          console.error('Error getting price history: ', error)
          return {
            error: message
          }
        }
      },
      providesTags: (_result, err, arg) =>
        err
          ? ['PriceHistory']
          : [
              {
                type: 'PriceHistory',
                id: `${arg.tokenParam}-${arg.vsAsset}-${arg.timeFrame}`
              }
            ]
    }),

    getPricesHistory: query<TokenPriceHistory[], GetPricesHistoryArg>({
      queryFn: async (
        { tokens, vsAsset, timeframe, tokenBalancesRegistry },
        { dispatch },
        extraOptions,
        baseQuery
      ) => {
        try {
          const {
            data: { assetRatioService }
          } = baseQuery(undefined)

          // dedupe tokens to prevent duplicate price history requests
          const uniqueIds = [...new Set(tokens.map(getPriceIdForToken))]
            // skip flagged coins such as testnet coins
            .filter((id) => id !== SKIP_PRICE_LOOKUP_COINGECKO_ID)

          const history = await mapLimit(
            uniqueIds,
            maxConcurrentPriceRequests,
            async function (id: string) {
              const { success, values } =
                await assetRatioService.getPriceHistory(id, vsAsset, timeframe)
              return {
                id,
                values: success
                  ? values.map((value) => ({
                      date: makeSerializableTimeDelta(value.date),
                      close: Number(value.price)
                    }))
                  : []
              }
            }
          )

          const aggregatedBalances: Record<string, ChainBalances> = {}
          for (const chainIds of Object.values(
            tokenBalancesRegistry.accounts
          )) {
            for (const [chainId, tokenBalancesForChainId] of Object.entries(
              chainIds.chains
            )) {
              if (!aggregatedBalances[chainId]) {
                aggregatedBalances[chainId] = { tokenBalances: {} }
              }

              const chainBalances = aggregatedBalances[chainId].tokenBalances

              for (const [assetId, tokenBalance] of Object.entries(
                tokenBalancesForChainId.tokenBalances
              )) {
                if (!chainBalances[assetId.toLowerCase()]) {
                  chainBalances[assetId.toLowerCase()] = tokenBalance
                } else {
                  chainBalances[assetId.toLowerCase()] = new Amount(
                    tokenBalance
                  )
                    .plus(chainBalances[assetId.toLowerCase()])
                    .format()
                }
              }
            }
          }

          const jointHistory = Object.entries(aggregatedBalances)
            .map(([chainId, tokenBalancesForChainId]) => {
              return Object.entries(tokenBalancesForChainId.tokenBalances).map(
                ([assetId, balance]) => {
                  const token = findTokenByAssetId(assetId, tokens)

                  if (token) {
                    const priceId = getPriceIdForToken(token)
                    const priceHistory = history.find((h) => h.id === priceId)
                    if (priceHistory) {
                      return priceHistory.values.map((v) => ({
                        date: v.date,
                        price: new Amount(balance)
                          .divideByDecimals(token.decimals)
                          .times(v.close)
                          .toNumber()
                      }))
                    }
                  }

                  return []
                }
              )
            })
            .flat(1)
            .filter((h) => h.length > 1)

          // Since the Price History API sometimes will return a shorter
          // array of history, this checks for the shortest array first to
          // then map and reduce to it length
          const shortestHistory =
            jointHistory.length > 0
              ? jointHistory.reduce((a, b) => (a.length <= b.length ? a : b))
              : []
          const sumOfHistory =
            jointHistory.length > 0
              ? shortestHistory.map((token, tokenIndex) => {
                  return {
                    date: token.date,
                    close: jointHistory
                      .map((price) => Number(price[tokenIndex].price) || 0)
                      .reduce((sum, x) => sum + x, 0)
                  }
                })
              : []

          return {
            data: sumOfHistory
          }
        } catch (error) {
          console.error(error)
          return {
            error: `Unable to fetch prices history`
          }
        }
      },
      providesTags: (_result, err, { tokens, vsAsset, timeframe }) =>
        err
          ? ['PricesHistory']
          : tokens.map((token) => ({
              type: 'PricesHistory',
              id: `${getPriceIdForToken(token)}-${vsAsset}-${timeframe}`
            }))
    })
  }
}
