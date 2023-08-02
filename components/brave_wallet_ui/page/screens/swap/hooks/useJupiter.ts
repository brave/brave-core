// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useMemo, useState } from 'react'

// Types
import { QuoteOption, SwapParams, SwapFee } from '../constants/types'
import {
  BraveWallet,
  CoinType
} from '../../../../constants/types'

// Constants
import { WRAPPED_SOL_CONTRACT_ADDRESS } from '../constants/magics'

// Utils
import Amount from '../../../../utils/amount'
import { makeNetworkAsset } from '../../../../options/asset-options'
import {
  getTokenPriceAmountFromRegistry
} from '../../../../utils/pricing-utils'

// Hooks
import { useLib } from '../../../../common/hooks/useLib'

// Query hooks
import {
  useGetSelectedChainQuery,
  useGetDefaultFiatCurrencyQuery
} from '../../../../common/slices/api.slice'
import { useSelectedAccountQuery } from '../../../../common/slices/api.slice.extra'

const networkFee = new Amount('0.000005')

export function useJupiter (params: SwapParams) {
  // Queries
  // FIXME(onyb): what happens when defaultFiatCurrency is empty
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAccount } = useSelectedAccountQuery()
  const nativeAsset = useMemo(() =>
    makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )

  // State
  const [quote, setQuote] = useState<BraveWallet.JupiterQuote | undefined>(undefined)
  const [error, setError] = useState<BraveWallet.JupiterErrorResponse | undefined>(undefined)
  const [loading, setLoading] = useState<boolean>(false)
  const [selectedRoute, setSelectedRoute] = useState<BraveWallet.JupiterRoute | undefined>(
    undefined
  )
  const [braveFee, setBraveFee] = useState<SwapFee | undefined>(undefined)
  const [abortController, setAbortController] = useState<AbortController | undefined>(undefined)

  // Custom hooks
  // FIXME(josheleonard): use slices API
  const { getSwapService, sendSolanaSerializedTransaction } = useLib()
  const swapService = getSwapService()

  // FIXME(josheleonard): convert to slices
  const getBraveFeeForAsset = useCallback(
    async (token: BraveWallet.BlockchainToken) => {
      if (token.coin !== CoinType.SOL) {
        throw Error('Unsupported coin type')
      }

      const hasFee = (await swapService.hasJupiterFeesForTokenMint(token.contractAddress)).result

      return {
        fee: '0.85',
        discount: hasFee ? '0' : '100'
      } as SwapFee
    },
    [swapService]
  )

  const reset = useCallback(
    async (callback?: () => Promise<void>) => {
      setQuote(undefined)
      setError(undefined)
      setLoading(false)
      setSelectedRoute(undefined)
      setBraveFee(undefined)

      if (abortController) {
        abortController.abort()
      }

      if (callback) {
        await callback()
      }
    },
    [abortController]
  )

  const refresh = useCallback(
    async function (
      overrides: Partial<SwapParams> = {}
    ): Promise<BraveWallet.JupiterQuote | undefined> {
      const overriddenParams: SwapParams = {
        ...params,
        ...overrides
      }

      // Perform data validation and early-exit
      if (selectedNetwork?.coin !== CoinType.SOL) {
        return
      }
      if (!overriddenParams.fromToken || !overriddenParams.toToken) {
        return
      }
      if (!overriddenParams.fromAmount && !overriddenParams.toAmount) {
        await reset()
        return
      }

      const fromAmountWrapped = new Amount(overriddenParams.fromAmount)
      const toAmountWrapped = new Amount(overriddenParams.toAmount)
      const isFromAmountEmpty =
        fromAmountWrapped.isZero() || fromAmountWrapped.isNaN() || fromAmountWrapped.isUndefined()
      const isToAmountEmpty =
        toAmountWrapped.isZero() || toAmountWrapped.isNaN() || toAmountWrapped.isUndefined()

      if (isFromAmountEmpty && isToAmountEmpty) {
        await reset()
        return
      }

      if (!overriddenParams.fromAddress) {
        return
      }

      const controller = new AbortController()
      setAbortController(controller)

      setLoading(true)

      try {
        const fee = await getBraveFeeForAsset(overriddenParams.toToken)
        setBraveFee(fee)
      } catch (e) {
        console.log(
          `Error getting Brave fee (Jupiter): ${
            //
            overriddenParams.toToken.symbol
          }`
        )
      }

      let jupiterQuoteResponse
      try {
        jupiterQuoteResponse = await swapService.getJupiterQuote({
          inputMint: overriddenParams.fromToken.contractAddress || WRAPPED_SOL_CONTRACT_ADDRESS,
          outputMint: overriddenParams.toToken.contractAddress || WRAPPED_SOL_CONTRACT_ADDRESS,
          amount: !isFromAmountEmpty
            ? new Amount(overriddenParams.fromAmount)
              .multiplyByDecimals(overriddenParams.fromToken.decimals)
              .format()
            : new Amount(overriddenParams.toAmount)
              .multiplyByDecimals(overriddenParams.toToken.decimals)
              .format(),
          slippageBps: new Amount(overriddenParams.slippageTolerance)
            .times(100)
            .parseInteger()
            .toNumber(),
          userPublicKey: overriddenParams.fromAddress
        })
      } catch (e) {
        console.log(`Error getting Jupiter quote: ${e}`)
      }

      if (controller.signal.aborted) {
        setLoading(false)
        setAbortController(undefined)
        return
      }

      if (jupiterQuoteResponse?.response) {
        setQuote(jupiterQuoteResponse.response)
      }

      if (jupiterQuoteResponse?.errorResponse) {
        setError(jupiterQuoteResponse.errorResponse)
      }

      setLoading(false)
      setAbortController(undefined)

      // Return undefined if response is null.
      return jupiterQuoteResponse?.response || undefined
    },
    [selectedNetwork?.coin, params, reset, swapService, getBraveFeeForAsset]
  )

  const exchange = useCallback(
    async function (callback?: () => Promise<void>) {
      // Perform data validation and early-exit
      if (!quote || quote?.routes.length === 0) {
        return
      }
      if (selectedNetwork?.coin !== CoinType.SOL) {
        return
      }
      if (!params.toToken) {
        return
      }
      if (!selectedAccount) {
        return
      }

      setLoading(true)
      let jupiterTransactionsPayloadResponse
      try {
        jupiterTransactionsPayloadResponse = await swapService.getJupiterSwapTransactions({
          userPublicKey: selectedAccount.address,
          route: selectedRoute || quote.routes[0],
          outputMint: params.toToken.contractAddress || WRAPPED_SOL_CONTRACT_ADDRESS
        })
      } catch (e) {
        console.log(`Error getting Jupiter swap transactions: ${e}`)
      }

      if (jupiterTransactionsPayloadResponse?.errorResponse) {
        setError(jupiterTransactionsPayloadResponse.errorResponse)
      }

      if (!jupiterTransactionsPayloadResponse?.response) {
        setLoading(false)
        return
      }

      const { swapTransaction } = jupiterTransactionsPayloadResponse.response

      try {
        const { success, errorMessage } = await sendSolanaSerializedTransaction({
          encodedTransaction: swapTransaction,
          from: selectedAccount.address,
          txType: BraveWallet.TransactionType.SolanaSwap,
          sendOptions: {
            skipPreflight: {
              skipPreflight: true
            },
            maxRetries: {
              maxRetries: BigInt(2)
            },
            preflightCommitment: undefined
          }
        })

        if (!success) {
          console.error(`Error creating Solana transaction: ${errorMessage}`)
        }

        await reset(callback)
      } catch (e) {
        // Bubble up error
        console.error(`Error creating Solana transaction: ${e}`)
        setLoading(false)
      }
    },
    [
      quote,
      selectedNetwork?.coin,
      params.toToken,
      selectedAccount,
      swapService,
      selectedRoute,
      sendSolanaSerializedTransaction,
      reset
    ]
  )

  const quoteOptions: QuoteOption[] = useMemo(() => {
    if (!params.fromToken || !params.toToken || !params.spotPrices) {
      return []
    }

    if (quote === undefined) {
      return []
    }

    return quote.routes.map(
      route =>
        ({
          label: route.marketInfos.map(marketInfo => marketInfo.label).join(' x '),
          fromAmount: new Amount(route.inAmount.toString()).divideByDecimals(
            // @ts-expect-error
            params.fromToken.decimals
          ),
          toAmount: new Amount(route.outAmount.toString()).divideByDecimals(
            // @ts-expect-error
            params.toToken.decimals
          ),
          // TODO: minimumToAmount is applicable only for ExactIn swapMode.
          // Create a maximumFromAmount field for ExactOut swapMode if needed.
          minimumToAmount: new Amount(route.otherAmountThreshold.toString()).divideByDecimals(
            // @ts-expect-error
            params.toToken.decimals
          ),
          fromToken: params.fromToken,
          toToken: params.toToken,
          rate: new Amount(route.outAmount.toString())
            // @ts-expect-error
            .divideByDecimals(params.toToken.decimals)
            .div(
              new Amount(route.inAmount.toString())
                // @ts-expect-error
                .divideByDecimals(params.fromToken.decimals)
            ),
          impact: new Amount(route.priceImpactPct),
          sources: route.marketInfos.flatMap(marketInfo =>
            // Split "Cykura (95%) + Lifinity (5%)"
            // into "Cykura (95%)" and "Lifinity (5%)"
            marketInfo.label.split('+').map(label => {
              // Extract name and proportion from Cykura (95%)
              const match = label.match(/([\W\s]+)\s+\((\d+)%\)/)
              if (match && match.length === 3) {
                return {
                  name: match[1].trim(),
                  proportion: new Amount(match[2]).div(100)
                }
              }

              return {
                name: label.trim(),
                proportion: new Amount(1)
              }
            })
          ),
          routing: route.marketInfos.length > 1 ? 'flow' : 'split',
          networkFee: networkFee
            .times(
              nativeAsset && params.spotPrices
                ? getTokenPriceAmountFromRegistry(
                    params.spotPrices, nativeAsset
                  )
                : Amount.zero()
            )
            .formatAsFiat(defaultFiatCurrency),
          braveFee
        } as QuoteOption)
    )
  }, [
    quote,
    params.fromToken,
    params.toToken,
    defaultFiatCurrency,
    nativeAsset,
    params.spotPrices,
    braveFee
  ])

  return {
    quote,
    error,
    loading,
    exchange,
    refresh,
    reset,
    selectedRoute,
    setSelectedRoute,
    quoteOptions,
    networkFee
  }
}
