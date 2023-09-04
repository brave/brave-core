// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useMemo, useState } from 'react'

// Types / constants
import { QuoteOption, SwapParams } from '../constants/types'
import { BraveWallet } from '../../../../constants/types'

import { MAX_UINT256, NATIVE_ASSET_CONTRACT_ADDRESS_0X } from '../constants/magics'

// Utils
import Amount from '../../../../utils/amount'
import { hexStrToNumberArray } from '../../../../utils/hex-utils'
import { getTokenPriceAmountFromRegistry } from '../../../../utils/pricing-utils'
import { makeNetworkAsset } from '../../../../options/asset-options'

// Query hooks
import {
  useGetSelectedChainQuery,
  useGetDefaultFiatCurrencyQuery
} from '../../../../common/slices/api.slice'
import { useSelectedAccountQuery } from '../../../../common/slices/api.slice.extra'
import { useLib } from '../../../../common/hooks/useLib'

export function useZeroEx (params: SwapParams) {
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
  const [quote, setQuote] = useState<BraveWallet.SwapResponse | undefined>(undefined)
  const [error, setError] = useState<BraveWallet.SwapErrorResponse | undefined>(undefined)
  const [hasAllowance, setHasAllowance] = useState<boolean>(false)
  const [loading, setLoading] = useState<boolean>(false)
  const [braveFee, setBraveFee] =
    useState<BraveWallet.BraveSwapFeeResponse | undefined>(undefined)
  const [abortController, setAbortController] = useState<AbortController | undefined>(undefined)

  // Custom hooks
  // FIXME(josheleonard): use slices API
  const { getERC20Allowance, getEthTxManagerProxy, getSwapService, sendEthTransaction } = useLib()
  const swapService = getSwapService()
  const ethTxManagerProxy = getEthTxManagerProxy()

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
    ): Promise<BraveWallet.SwapResponse | undefined> {
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

      let priceQuoteResponse
      try {
        priceQuoteResponse = await swapService.getPriceQuote({
          takerAddress: overriddenParams.fromAddress,
          sellAmount:
            overriddenParams.fromAmount &&
            new Amount(overriddenParams.fromAmount)
              .multiplyByDecimals(overriddenParams.fromToken.decimals)
              .format(),
          sellToken: overriddenParams.fromToken.contractAddress || NATIVE_ASSET_CONTRACT_ADDRESS_0X,
          buyAmount:
            overriddenParams.toAmount &&
            new Amount(overriddenParams.toAmount)
              .multiplyByDecimals(overriddenParams.toToken.decimals)
              .format(),
          buyToken: overriddenParams.toToken.contractAddress || NATIVE_ASSET_CONTRACT_ADDRESS_0X,
          slippagePercentage: new Amount(overriddenParams.slippageTolerance).div(100).toNumber(),
          gasPrice: ''
        })
      } catch (e) {
        console.log(`Error getting 0x quote: ${e}`)
      }

      try {
        const { response: braveFeeResponse } = await swapService.getBraveFee({
          chainId: selectedNetwork.chainId,
          inputToken:
            overriddenParams.fromToken.contractAddress ||
            NATIVE_ASSET_CONTRACT_ADDRESS_0X,
          outputToken:
            overriddenParams.toToken.contractAddress ||
            NATIVE_ASSET_CONTRACT_ADDRESS_0X,
          taker: overriddenParams.fromAddress
        })

        if (priceQuoteResponse?.response && braveFeeResponse) {
          setBraveFee({
            ...braveFeeResponse,
            protocolFeePct: priceQuoteResponse.response.fees.zeroExFee
              ? braveFeeResponse.protocolFeePct
              : '0'
          })
        }
      } catch (e) {
        console.log(
          `Error getting Brave fee (Jupiter):
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
        priceQuoteResponse?.response &&
        overriddenParams.fromToken.contractAddress
      ) {
        try {
          const allowance = await getERC20Allowance(
            priceQuoteResponse.response.sellTokenAddress,
            selectedAccount.address,
            priceQuoteResponse.response.allowanceTarget,
            selectedNetwork.chainId,
          )
          hasAllowanceResult = new Amount(allowance).gte(priceQuoteResponse.response.sellAmount)
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

      if (priceQuoteResponse?.response) {
        setQuote(priceQuoteResponse.response)
      }

      if (priceQuoteResponse?.errorResponse) {
        setError(priceQuoteResponse.errorResponse)
      }

      setHasAllowance(hasAllowanceResult)

      setLoading(false)
      setAbortController(undefined)

      // Return undefined if response is null.
      return priceQuoteResponse?.response || undefined
    },
    [
      params,
      selectedNetwork?.coin,
      selectedAccount,
      reset,
      swapService,
      getERC20Allowance
    ]
  )

  const exchange = useCallback(
    async function (overrides: Partial<SwapParams> = {}, callback?: () => Promise<void>) {
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
        fromAmountWrapped.isZero() || fromAmountWrapped.isNaN() || fromAmountWrapped.isUndefined()
      const isToAmountEmpty =
        toAmountWrapped.isZero() || toAmountWrapped.isNaN() || toAmountWrapped.isUndefined()

      if (isFromAmountEmpty && isToAmountEmpty) {
        return
      }

      if (!overriddenParams.fromAddress) {
        return
      }

      setLoading(true)
      let transactionPayloadResponse
      try {
        transactionPayloadResponse = await swapService.getTransactionPayload({
          takerAddress: overriddenParams.fromAddress,
          sellAmount: new Amount(overriddenParams.fromAmount)
            .multiplyByDecimals(overriddenParams.fromToken.decimals)
            .format(),
          sellToken: overriddenParams.fromToken.contractAddress || NATIVE_ASSET_CONTRACT_ADDRESS_0X,
          buyAmount: new Amount(overriddenParams.toAmount)
            .multiplyByDecimals(overriddenParams.toToken.decimals)
            .format(),
          buyToken: overriddenParams.toToken.contractAddress || NATIVE_ASSET_CONTRACT_ADDRESS_0X,
          slippagePercentage: new Amount(overriddenParams.slippageTolerance).div(100).toNumber(),
          gasPrice: ''
        })
      } catch (e) {
        console.log(`Error getting 0x swap quote: ${e}`)
      }

      if (transactionPayloadResponse?.errorResponse) {
        setError(transactionPayloadResponse?.errorResponse)
      }

      if (!transactionPayloadResponse?.response) {
        setLoading(false)
        return
      }

      const { data, to, value, estimatedGas } = transactionPayloadResponse.response

      try {
        await sendEthTransaction({
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
    [params, selectedNetwork, selectedAccount, swapService, sendEthTransaction, reset]
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
      const { success, data } = await ethTxManagerProxy.makeERC20ApproveData(
        allowanceTarget,
        // FIXME(onyb): reduce allowance to the minimum required amount
        // for security reasons.
        new Amount(MAX_UINT256).toHex()
      )

      if (!success) {
        console.error(`Error creating ERC20 approve data.`)
        return
      }

      await sendEthTransaction({
        fromAccount: selectedAccount,
        to: sellTokenAddress,
        value: '0x0',
        data,
        network: selectedNetwork
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
    sendEthTransaction,
    ethTxManagerProxy.makeERC20ApproveData
  ])

  const networkFee = useMemo(() => {
    if (!quote || !selectedNetwork?.decimals) {
      return Amount.empty()
    }

    return new Amount(quote.gasPrice).times(quote.gas).divideByDecimals(selectedNetwork.decimals)
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
