// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useMemo, useState } from 'react'

// Types
import { QuoteOption, SwapParams } from '../constants/types'
import { BraveWallet } from '../../../../constants/types'

// Constants
import { WRAPPED_SOL_CONTRACT_ADDRESS } from '../constants/magics'

// Utils
import Amount from '../../../../utils/amount'
import { makeNetworkAsset } from '../../../../options/asset-options'
import { getTokenPriceAmountFromRegistry } from '../../../../utils/pricing-utils'
import { toMojoUnion } from '../../../../utils/mojo-utils'

// Hooks
import { useLib } from '../../../../common/hooks/useLib'

// Query hooks
import {
  useGetDefaultFiatCurrencyQuery //
} from '../../../../common/slices/api.slice'

const networkFee = new Amount('0.000005')

export function useJupiter(params: SwapParams) {
  const { selectedNetwork, selectedAccount } = params

  // Queries
  // FIXME(onyb): what happens when defaultFiatCurrency is empty
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const nativeAsset = useMemo(
    () => makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )

  // State
  const [quote, setQuote] = useState<BraveWallet.JupiterQuote | undefined>(
    undefined
  )
  const [error, setError] = useState<BraveWallet.JupiterError | undefined>(
    undefined
  )
  const [loading, setLoading] = useState<boolean>(false)
  const [selectedRoute, setSelectedRoute] = useState<
    BraveWallet.JupiterRoute | undefined
  >(undefined)
  const [braveFee, setBraveFee] = useState<
    BraveWallet.BraveSwapFeeResponse | undefined
  >(undefined)
  const [abortController, setAbortController] = useState<
    AbortController | undefined
  >(undefined)

  // Custom hooks
  // FIXME(josheleonard): use slices API
  const { getSwapService, sendSolanaSerializedTransaction } = useLib()
  const swapService = getSwapService()

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
      if (
        !selectedNetwork ||
        selectedNetwork.coin !== BraveWallet.CoinType.SOL
      ) {
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
        fromAmountWrapped.isZero() ||
        fromAmountWrapped.isNaN() ||
        fromAmountWrapped.isUndefined()
      const isToAmountEmpty =
        toAmountWrapped.isZero() ||
        toAmountWrapped.isNaN() ||
        toAmountWrapped.isUndefined()

      if (isFromAmountEmpty && isToAmountEmpty) {
        await reset()
        return
      }

      if (!overriddenParams.fromAccount) {
        return
      }

      const controller = new AbortController()
      setAbortController(controller)

      setLoading(true)

      try {
        const { response: braveFeeResponse } = await swapService.getBraveFee({
          chainId: selectedNetwork.chainId,
          inputToken:
            overriddenParams.fromToken.contractAddress ||
            WRAPPED_SOL_CONTRACT_ADDRESS,
          outputToken:
            overriddenParams.toToken.contractAddress ||
            WRAPPED_SOL_CONTRACT_ADDRESS,
          taker: overriddenParams.fromAccount.address
        })
        setBraveFee(braveFeeResponse || undefined)
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
        jupiterQuoteResponse = await swapService.getQuote({
          fromAccountId: overriddenParams.fromAccount.accountId,
          fromChainId: selectedNetwork.chainId,
          fromToken:
            overriddenParams.fromToken.contractAddress ||
            WRAPPED_SOL_CONTRACT_ADDRESS,
          fromAmount: isFromAmountEmpty
            ? new Amount(overriddenParams.toAmount)
                .multiplyByDecimals(overriddenParams.toToken.decimals)
                .format()
            : new Amount(overriddenParams.fromAmount)
                .multiplyByDecimals(overriddenParams.fromToken.decimals)
                .format(),
          toAccountId: overriddenParams.fromAccount.accountId,
          toChainId: selectedNetwork.chainId,
          toToken:
            overriddenParams.toToken.contractAddress ||
            WRAPPED_SOL_CONTRACT_ADDRESS,
          toAmount: '',
          slippagePercentage: overriddenParams.slippageTolerance,
          routePriority: BraveWallet.RoutePriority.kRecommended
        })
      } catch (e) {
        console.log(`Error getting Jupiter quote: ${e}`)
      }

      if (controller.signal.aborted) {
        setLoading(false)
        setAbortController(undefined)
        return
      }

      if (jupiterQuoteResponse?.response?.jupiterQuote) {
        setQuote(jupiterQuoteResponse.response.jupiterQuote)
      }

      if (jupiterQuoteResponse?.error?.jupiterError) {
        setError(jupiterQuoteResponse.error.jupiterError)
      }

      setLoading(false)
      setAbortController(undefined)

      // Return undefined if response is null.
      return jupiterQuoteResponse?.response?.jupiterQuote || undefined
    },
    [selectedNetwork, params, reset, swapService]
  )

  const exchange = useCallback(
    async function (callback?: () => Promise<void>) {
      // Perform data validation and early-exit
      if (!quote || quote?.routes.length === 0) {
        return
      }
      if (selectedNetwork?.coin !== BraveWallet.CoinType.SOL) {
        return
      }
      if (!params.toToken || !params.fromToken) {
        return
      }
      if (!selectedAccount) {
        return
      }

      setLoading(true)
      let jupiterTransactionResponse
      try {
        jupiterTransactionResponse = await swapService.getTransaction(
          toMojoUnion(
            {
              jupiterTransactionParams: {
                chainId: selectedNetwork.chainId,
                userPublicKey: selectedAccount.address,
                route: selectedRoute || quote.routes[0],
                inputMint:
                  params.fromToken.contractAddress ||
                  WRAPPED_SOL_CONTRACT_ADDRESS,
                outputMint:
                  params.toToken.contractAddress || WRAPPED_SOL_CONTRACT_ADDRESS
              },
              zeroExTransactionParams: undefined
            },
            'jupiterTransactionParams'
          )
        )
      } catch (e) {
        console.log(`Error getting Jupiter swap transactions: ${e}`)
      }

      if (jupiterTransactionResponse?.error?.jupiterError) {
        setError(jupiterTransactionResponse.error.jupiterError)
      }

      if (!jupiterTransactionResponse?.response?.jupiterTransaction) {
        setLoading(false)
        return
      }

      const swapTransaction =
        jupiterTransactionResponse.response.jupiterTransaction

      try {
        const { success, errorMessage } = await sendSolanaSerializedTransaction(
          {
            encodedTransaction: swapTransaction,
            chainId: selectedNetwork.chainId,
            accountId: selectedAccount.accountId,
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
          }
        )

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
      selectedNetwork,
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
      (route) =>
        ({
          label: route.marketInfos
            .map((marketInfo) => marketInfo.label)
            .join(' x '),
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
          minimumToAmount: new Amount(
            route.otherAmountThreshold.toString()
          ).divideByDecimals(
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
          sources: route.marketInfos.flatMap((marketInfo) =>
            // Split "Cykura (95%) + Lifinity (5%)"
            // into "Cykura (95%)" and "Lifinity (5%)"
            marketInfo.label.split('+').map((label) => {
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
                    params.spotPrices,
                    nativeAsset
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
