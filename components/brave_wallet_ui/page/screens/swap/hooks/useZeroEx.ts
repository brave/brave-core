// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useMemo, useState } from 'react'

// Types / constants
import { QuoteOption, SwapParams } from '../constants/types'
import { BraveWallet } from '../../../../constants/types'

import { MAX_UINT256 } from '../constants/magics'
import { NATIVE_EVM_ASSET_CONTRACT_ADDRESS } from '../../../../common/constants/magics'

// Utils
import Amount from '../../../../utils/amount'
import { hexStrToNumberArray } from '../../../../utils/hex-utils'
import { getTokenPriceAmountFromRegistry } from '../../../../utils/pricing-utils'
import { makeNetworkAsset } from '../../../../options/asset-options'
import { toMojoUnion } from '../../../../utils/mojo-utils'

// Query hooks
import {
  useApproveERC20AllowanceMutation,
  useGenerateBraveSwapFeeMutation,
  useGenerateSwapQuoteMutation,
  useGenerateSwapTransactionMutation,
  useGetDefaultFiatCurrencyQuery,
  useLazyGetERC20AllowanceQuery,
  useSendEvmTransactionMutation
} from '../../../../common/slices/api.slice'

export function useZeroEx(params: SwapParams) {
  const { selectedNetwork, selectedAccount } = params

  // Queries
  const [getERC20Allowance] = useLazyGetERC20AllowanceQuery()
  // FIXME(onyb): what happens when defaultFiatCurrency is empty
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const nativeAsset = useMemo(
    () => makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )

  // Mutations
  const [sendEvmTransaction] = useSendEvmTransactionMutation()
  const [approveERC20Allowance] = useApproveERC20AllowanceMutation()
  const [generateSwapQuote] = useGenerateSwapQuoteMutation()
  const [generateBraveSwapFee] = useGenerateBraveSwapFeeMutation()
  const [generateSwapTransaction] = useGenerateSwapTransactionMutation()

  // State
  const [quote, setQuote] = useState<BraveWallet.ZeroExQuote | undefined>(
    undefined
  )
  const [error, setError] = useState<BraveWallet.ZeroExError | undefined>(
    undefined
  )
  const [hasAllowance, setHasAllowance] = useState<boolean>(false)
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
    ): Promise<BraveWallet.ZeroExQuote | undefined> {
      const overriddenParams: SwapParams = {
        ...params,
        ...overrides
      }

      // Perform data validation and early-exit
      if (selectedNetwork?.coin !== BraveWallet.CoinType.ETH) {
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

      let quoteResponse
      try {
        quoteResponse = await generateSwapQuote({
          fromAccountId: overriddenParams.fromAccount.accountId,
          fromChainId: selectedNetwork.chainId,
          fromAmount:
            overriddenParams.fromAmount &&
            new Amount(overriddenParams.fromAmount)
              .multiplyByDecimals(overriddenParams.fromToken.decimals)
              .format(),
          fromToken:
            overriddenParams.fromToken.contractAddress ||
            NATIVE_EVM_ASSET_CONTRACT_ADDRESS,

          toAccountId: overriddenParams.fromAccount.accountId,
          toChainId: selectedNetwork.chainId,
          toAmount:
            overriddenParams.toAmount &&
            new Amount(overriddenParams.toAmount)
              .multiplyByDecimals(overriddenParams.toToken.decimals)
              .format(),
          toToken:
            overriddenParams.toToken.contractAddress ||
            NATIVE_EVM_ASSET_CONTRACT_ADDRESS,
          slippagePercentage: overriddenParams.slippageTolerance,
          routePriority: BraveWallet.RoutePriority.kRecommended
        }).unwrap()
      } catch (e) {
        console.log(`Error getting 0x quote: ${e}`)
      }

      try {
        const { response: braveFeeResponse } = await generateBraveSwapFee({
          chainId: selectedNetwork.chainId,
          inputToken:
            overriddenParams.fromToken.contractAddress ||
            NATIVE_EVM_ASSET_CONTRACT_ADDRESS,
          outputToken:
            overriddenParams.toToken.contractAddress ||
            NATIVE_EVM_ASSET_CONTRACT_ADDRESS,
          taker: overriddenParams.fromAccount.address
        }).unwrap()

        if (quoteResponse?.response?.zeroExQuote && braveFeeResponse) {
          setBraveFee({
            ...braveFeeResponse,
            protocolFeePct: quoteResponse.response.zeroExQuote.fees.zeroExFee
              ? braveFeeResponse.protocolFeePct
              : '0'
          })
        }
      } catch (e) {
        console.log(
          `Error getting Brave fee (0x):
          ${overriddenParams.toToken.symbol}`
        )
      }

      let hasAllowanceResult = false

      // Native asset does not have allowance requirements, so we always
      // default to true.
      if (!overriddenParams.fromToken.contractAddress) {
        hasAllowanceResult = true
      }

      // Simulate that the token has enough allowance if the wallet is not
      // connected yet.
      if (!selectedAccount) {
        hasAllowanceResult = true
      }

      if (
        selectedAccount &&
        quoteResponse?.response?.zeroExQuote &&
        overriddenParams.fromToken.contractAddress
      ) {
        try {
          const allowance = await getERC20Allowance({
            contractAddress:
              quoteResponse.response.zeroExQuote.sellTokenAddress,
            ownerAddress: selectedAccount.address,
            spenderAddress: quoteResponse.response.zeroExQuote.allowanceTarget,
            chainId: selectedNetwork.chainId
          }).unwrap()
          hasAllowanceResult = new Amount(allowance).gte(
            quoteResponse.response.zeroExQuote.sellAmount
          )
        } catch (e) {
          // bubble up error
          console.log(`Error getting ERC20 allowance: ${e}`)
        }
      }

      if (controller.signal.aborted) {
        setLoading(false)
        setAbortController(undefined)
        return
      }

      if (quoteResponse?.response?.zeroExQuote) {
        setQuote(quoteResponse.response.zeroExQuote)
      }

      if (quoteResponse?.error) {
        setError(quoteResponse.error.zeroExError)
      }

      setHasAllowance(hasAllowanceResult)

      setLoading(false)
      setAbortController(undefined)

      // Return undefined if response is null.
      return quoteResponse?.response?.zeroExQuote || undefined
    },
    [
      params,
      selectedNetwork,
      selectedAccount,
      reset,
      generateSwapQuote,
      generateBraveSwapFee,
      getERC20Allowance
    ]
  )

  const exchange = useCallback(
    async function (
      overrides: Partial<SwapParams> = {},
      callback?: () => Promise<void>
    ) {
      const overriddenParams: SwapParams = {
        ...params,
        ...overrides
      }

      // Perform data validation and early-exit
      if (selectedNetwork?.coin !== BraveWallet.CoinType.ETH) {
        return
      }
      if (!selectedAccount) {
        // Wallet is not connected
        return
      }
      if (!overriddenParams.fromToken || !overriddenParams.toToken) {
        return
      }
      if (!overriddenParams.fromAmount && !overriddenParams.toAmount) {
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
        return
      }

      if (!overriddenParams.fromAccount) {
        return
      }

      setLoading(true)
      let transactionResponse
      try {
        transactionResponse = await generateSwapTransaction(
          toMojoUnion(
            {
              zeroExTransactionParams: {
                fromAccountId: overriddenParams.fromAccount.accountId,
                fromChainId: selectedNetwork.chainId,
                fromAmount: new Amount(overriddenParams.fromAmount)
                  .multiplyByDecimals(overriddenParams.fromToken.decimals)
                  .format(),
                fromToken:
                  overriddenParams.fromToken.contractAddress ||
                  NATIVE_EVM_ASSET_CONTRACT_ADDRESS,

                toAccountId: overriddenParams.fromAccount.accountId,
                toChainId: selectedNetwork.chainId,
                toAmount: new Amount(overriddenParams.toAmount)
                  .multiplyByDecimals(overriddenParams.toToken.decimals)
                  .format(),
                toToken:
                  overriddenParams.toToken.contractAddress ||
                  NATIVE_EVM_ASSET_CONTRACT_ADDRESS,
                slippagePercentage: overriddenParams.slippageTolerance,
                routePriority: BraveWallet.RoutePriority.kRecommended
              },
              jupiterTransactionParams: undefined
            },
            'zeroExTransactionParams'
          )
        ).unwrap()
      } catch (e) {
        console.log(`Error getting 0x swap quote: ${e}`)
      }

      if (transactionResponse?.error?.zeroExError) {
        setError(transactionResponse?.error.zeroExError)
      }

      if (!transactionResponse?.response?.zeroExTransaction) {
        setLoading(false)
        return
      }

      const { data, to, value, estimatedGas } =
        transactionResponse.response.zeroExTransaction

      try {
        await sendEvmTransaction({
          fromAccount: selectedAccount,
          to,
          value: new Amount(value).toHex(),
          gas: new Amount(estimatedGas).toHex(),
          data: hexStrToNumberArray(data),
          network: selectedNetwork
        })

        await reset(callback)
      } catch (e) {
        // bubble up error
        console.error(`Error creating 0x transaction: ${e}`)
        setLoading(false)
      }
    },
    [
      params,
      selectedNetwork,
      selectedAccount,
      generateSwapTransaction,
      sendEvmTransaction,
      reset
    ]
  )

  const approve = useCallback(async () => {
    if (!quote || hasAllowance) {
      return
    }

    // Typically when wallet has not been connected yet
    if (!selectedAccount || !selectedNetwork) {
      return
    }

    const { allowanceTarget, sellTokenAddress } = quote
    try {
      await approveERC20Allowance({
        network: selectedNetwork,
        fromAccount: selectedAccount,
        contractAddress: sellTokenAddress,
        spenderAddress: allowanceTarget,

        // FIXME(onyb): reduce allowance to the minimum required amount
        // for security reasons.
        allowance: new Amount(MAX_UINT256).toHex()
      })
    } catch (e) {
      // bubble up error
      console.error(`Error creating ERC20 approve transaction: ${e}`)
    }
  }, [
    quote,
    hasAllowance,
    selectedAccount,
    selectedNetwork,
    approveERC20Allowance
  ])

  const networkFee = useMemo(() => {
    if (!quote || !selectedNetwork?.decimals) {
      return Amount.empty()
    }

    return new Amount(quote.gasPrice)
      .times(quote.gas)
      .divideByDecimals(selectedNetwork.decimals)
  }, [quote, selectedNetwork?.decimals])

  const quoteOptions: QuoteOption[] = useMemo(() => {
    if (
      !params.fromToken ||
      !params.toToken ||
      !params.spotPrices ||
      !nativeAsset
    ) {
      return []
    }

    if (quote === undefined) {
      return []
    }

    return [
      {
        label: '',
        fromAmount: new Amount(quote.sellAmount).divideByDecimals(
          params.fromToken.decimals
        ),
        toAmount: new Amount(quote.buyAmount).divideByDecimals(
          params.toToken.decimals
        ),
        minimumToAmount: undefined,
        fromToken: params.fromToken,
        toToken: params.toToken,
        rate: new Amount(quote.buyAmount)
          .divideByDecimals(params.toToken.decimals)
          .div(
            new Amount(quote.sellAmount).divideByDecimals(
              params.fromToken.decimals
            )
          ),
        impact: new Amount(quote.estimatedPriceImpact),
        sources: quote.sources
          .map((source) => ({
            name: source.name,
            proportion: new Amount(source.proportion)
          }))
          .filter((source) => source.proportion.gt(0)),
        routing: 'split', // 0x supports split routing only
        networkFee: networkFee.isUndefined()
          ? ''
          : networkFee
              .times(
                getTokenPriceAmountFromRegistry(params.spotPrices, nativeAsset)
              )
              .formatAsFiat(defaultFiatCurrency),
        braveFee
      }
    ]
  }, [
    params.fromToken,
    params.toToken,
    quote,
    defaultFiatCurrency,
    networkFee,
    params.spotPrices,
    braveFee,
    nativeAsset
  ])

  return {
    quote,
    error,
    hasAllowance,
    loading,
    exchange,
    refresh,
    reset,
    approve,
    quoteOptions,
    networkFee
  }
}
