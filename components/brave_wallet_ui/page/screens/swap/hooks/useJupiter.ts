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

// Query hooks
import {
  useGenerateBraveSwapFeeMutation,
  useGenerateSwapQuoteMutation,
  useGenerateSwapTransactionMutation,
  useGetDefaultFiatCurrencyQuery,
  useSendSolanaSerializedTransactionMutation
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

  // Mutations
  const [generateSwapQuote] = useGenerateSwapQuoteMutation()
  const [generateBraveSwapFee] = useGenerateBraveSwapFeeMutation()
  const [generateSwapTransaction] = useGenerateSwapTransactionMutation()
  const [sendSolanaSerializedTransaction] =
    useSendSolanaSerializedTransactionMutation()

  // State
  const [quote, setQuote] = useState<BraveWallet.JupiterQuote | undefined>(
    undefined
  )
  const [error, setError] = useState<BraveWallet.JupiterError | undefined>(
    undefined
  )
  const [loading, setLoading] = useState<boolean>(false)
  const [braveFee, setBraveFee] = useState<
    BraveWallet.BraveSwapFeeResponse | undefined
  >(undefined)
  const [abortController, setAbortController] = useState<
    AbortController | undefined
  >(undefined)

  const reset = useCallback(
    async (callback?: () => Promise<void>) => {
      setQuote(undefined)
      setError(undefined)
      setLoading(false)
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
        const { response: braveFeeResponse } = await generateBraveSwapFee({
          chainId: selectedNetwork.chainId,
          inputToken:
            overriddenParams.fromToken.contractAddress ||
            WRAPPED_SOL_CONTRACT_ADDRESS,
          outputToken:
            overriddenParams.toToken.contractAddress ||
            WRAPPED_SOL_CONTRACT_ADDRESS,
          taker: overriddenParams.fromAccount.address
        }).unwrap()
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
        jupiterQuoteResponse = await generateSwapQuote({
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
        }).unwrap()
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
    [selectedNetwork, params, reset, generateBraveSwapFee, generateSwapQuote]
  )

  const exchange = useCallback(
    async function (callback?: () => Promise<void>) {
      // Perform data validation and early-exit
      if (!quote || quote?.routePlan.length === 0) {
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
        jupiterTransactionResponse = await generateSwapTransaction(
          toMojoUnion(
            {
              jupiterTransactionParams: {
                chainId: selectedNetwork.chainId,
                userPublicKey: selectedAccount.address,
                quote
              },
              zeroExTransactionParams: undefined
            },
            'jupiterTransactionParams'
          )
        ).unwrap()
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
        await sendSolanaSerializedTransaction({
          encodedTransaction: swapTransaction,
          chainId: selectedNetwork.chainId,
          accountId: selectedAccount.accountId,
          txType: BraveWallet.TransactionType.SolanaSwap,
          sendOptions: {
            skipPreflight: {
              skipPreflight: true
            },
            maxRetries: {
              maxRetries: BigInt(3)
            },
            preflightCommitment: 'processed'
          }
        }).unwrap()
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
      generateSwapTransaction,
      sendSolanaSerializedTransaction,
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

    return [
      {
        label: '',
        fromAmount: new Amount(quote.inAmount).divideByDecimals(
          params.fromToken.decimals
        ),
        toAmount: new Amount(quote.outAmount).divideByDecimals(
          params.toToken.decimals
        ),
        // TODO: minimumToAmount is applicable only for ExactIn swapMode.
        // Create a maximumFromAmount field for ExactOut swapMode if needed.
        minimumToAmount: new Amount(
          quote.otherAmountThreshold
        ).divideByDecimals(params.toToken.decimals),
        fromToken: params.fromToken,
        toToken: params.toToken,
        rate: new Amount(quote.outAmount)
          .divideByDecimals(params.toToken.decimals)
          .div(
            new Amount(quote.inAmount).divideByDecimals(
              params.fromToken.decimals
            )
          ),
        impact: new Amount(quote.priceImpactPct),
        sources: [
          ...new Set(quote.routePlan.map((step) => step.swapInfo.label))
        ].map((name) => ({
          name,
          proportion: new Amount(1)
        })),
        // TODO(onyb): this is a placeholder value until we have a better
        // routing UI
        routing: 'flow',
        networkFee: networkFee
          .times(
            nativeAsset && params.spotPrices
              ? getTokenPriceAmountFromRegistry(params.spotPrices, nativeAsset)
              : Amount.zero()
          )
          .formatAsFiat(defaultFiatCurrency),
        braveFee
      } as QuoteOption
    ]
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
    quoteOptions,
    networkFee
  }
}
