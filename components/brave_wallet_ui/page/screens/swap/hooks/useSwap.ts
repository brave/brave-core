// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useEffect, useMemo, useState } from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Options
import { SwapAndSendOptions } from '../../../../options/swap-and-send-options'
import { gasFeeOptions } from '../../../../options/gas-fee-options'

// Hooks
import { useJupiter } from './useJupiter'
import { useZeroEx } from './useZeroEx'
import { useDebouncedCallback } from './useDebouncedCallback'
import {
  useBalancesFetcher //
} from '../../../../common/hooks/use-balances-fetcher'

// Types and constants
import {
  GasEstimate,
  SwapValidationErrorType,
  SwapParamsOverrides
} from '../constants/types'
import { BraveWallet, GasFeeOption } from '../../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import Amount from '../../../../utils/amount'
import { getPriceIdForToken } from '../../../../utils/api-utils'
// FIXME(onyb): move makeNetworkAsset to utils/assets-utils
import { makeNetworkAsset } from '../../../../options/asset-options'
import { getTokenPriceAmountFromRegistry } from '../../../../utils/pricing-utils'
import { getBalance } from '../../../../utils/balance-utils'
import { networkSupportsAccount } from '../../../../utils/network-utils'
import {
  getZeroExQuoteOptions,
  getJupiterQuoteOptions,
  getZeroExFromAmount,
  getZeroExToAmount,
  getJupiterFromAmount,
  getJupiterToAmount
} from '../swap.utils'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetSwapSupportedNetworksQuery,
  useGetTokenSpotPricesQuery,
  useGenerateSwapQuoteMutation
} from '../../../../common/slices/api.slice'
import { querySubscriptionOptions60s } from '../../../../common/slices/constants'
import { AccountInfoEntity } from '../../../../common/slices/entities/account-info.entity'

const hasDecimalsOverflow = (
  amount: string,
  asset?: BraveWallet.BlockchainToken
) => {
  if (!asset) {
    return false
  }

  const amountBaseWrapped = new Amount(amount).multiplyByDecimals(
    asset.decimals
  )
  if (!amountBaseWrapped.value) {
    return false
  }

  const decimalPlaces = amountBaseWrapped.value.decimalPlaces()
  return decimalPlaces !== null && decimalPlaces > 0
}

export const useSwap = () => {
  // State
  const [fromAccount, setFromAccount] = useState<
    BraveWallet.AccountInfo | undefined
  >(undefined)
  const [fromToken, setFromToken] = useState<
    BraveWallet.BlockchainToken | undefined
  >(undefined)
  const [toAccountId, setToAccountId] = useState<
    BraveWallet.AccountId | undefined
  >(undefined)
  const [toToken, setToToken] = useState<
    BraveWallet.BlockchainToken | undefined
  >(undefined)
  const [fromAmount, setFromAmount] = useState<string>('')
  const [toAmount, setToAmount] = useState<string>('')
  const [selectingFromOrTo, setSelectingFromOrTo] = useState<
    'from' | 'to' | undefined
  >(undefined)
  const [editingFromOrToAmount, setEditingFromOrToAmount] = useState<
    'from' | 'to'
  >('from')
  const [selectedSwapAndSendOption, setSelectedSwapAndSendOption] =
    useState<string>(SwapAndSendOptions[0].name)
  const [swapAndSendSelected, setSwapAndSendSelected] = useState<boolean>(false)
  const [toAnotherAddress, setToAnotherAddress] = useState<string>('')
  const [userConfirmedAddress, setUserConfirmedAddress] =
    useState<boolean>(false)
  const [useDirectRoute, setUseDirectRoute] = useState<boolean>(false)
  const [slippageTolerance, setSlippageTolerance] = useState<string>('0.5')
  const [selectedGasFeeOption, setSelectedGasFeeOption] =
    useState<GasFeeOption>(gasFeeOptions[1])
  const [quoteUnion, setQuoteUnion] = useState<
    BraveWallet.SwapQuoteUnion | undefined
  >(undefined)
  const [quoteErrorUnion, setQuoteErrorUnion] = useState<
    BraveWallet.SwapErrorUnion | undefined
  >(undefined)
  const [abortController, setAbortController] = useState<
    AbortController | undefined
  >(undefined)
  const [isFetchingQuote, setIsFetchingQuote] = useState<boolean>(false)
  const [swapFees, setSwapFees] = useState<BraveWallet.SwapFees | undefined>(
    undefined
  )

  // Mutations
  const [generateSwapQuote] = useGenerateSwapQuoteMutation()

  // Queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: supportedNetworks } = useGetSwapSupportedNetworksQuery()

  // Memos
  const fromNetwork = useMemo(() => {
    if (fromToken && supportedNetworks?.length) {
      return supportedNetworks.find(
        (network) =>
          network.chainId === fromToken.chainId &&
          network.coin === fromToken.coin
      )
    }

    return undefined
  }, [fromToken, supportedNetworks])

  const toNetwork = useMemo(() => {
    if (toToken && supportedNetworks?.length) {
      return supportedNetworks.find(
        (network) =>
          network.chainId === toToken.chainId && network.coin === toToken.coin
      )
    }

    return fromNetwork
  }, [toToken, supportedNetworks, fromNetwork])

  const [selectedSwapSendAccount, setSelectedSwapSendAccount] = useState<
    AccountInfoEntity | undefined
  >(undefined)

  const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
    useBalancesFetcher(
      fromNetwork && fromAccount
        ? {
            networks: [fromNetwork],
            accounts: [fromAccount]
          }
        : skipToken
    )

  const nativeAsset = useMemo(
    () => makeNetworkAsset(fromNetwork),
    [fromNetwork]
  )

  const tokenPriceIds = useMemo(
    () =>
      [nativeAsset, fromToken, toToken]
        .filter(
          (token): token is BraveWallet.BlockchainToken => token !== undefined
        )
        .map(getPriceIdForToken),
    [nativeAsset, fromToken, toToken]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiatCurrency
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  const jupiter = useJupiter({
    fromNetwork,
    fromAccount,
    fromToken,
    fromAmount: editingFromOrToAmount === 'from' ? fromAmount : '',
    toAccountId,
    toToken,
    toAmount: editingFromOrToAmount === 'to' ? toAmount : '',
    slippageTolerance
  })
  const zeroEx = useZeroEx({
    fromNetwork,
    fromAccount,
    fromToken,
    fromAmount: editingFromOrToAmount === 'from' ? fromAmount : '',
    toAccountId,
    toToken,
    toAmount: editingFromOrToAmount === 'to' ? toAmount : '',
    slippageTolerance
  })

  const quoteOptions = useMemo(() => {
    if (
      !fromNetwork ||
      !fromToken ||
      !toToken ||
      !quoteUnion ||
      !spotPriceRegistry ||
      !defaultFiatCurrency
    ) {
      return []
    }

    if (quoteUnion.jupiterQuote) {
      return getJupiterQuoteOptions({
        quote: quoteUnion.jupiterQuote,
        fromNetwork,
        fromToken,
        toToken,
        spotPrices: spotPriceRegistry,
        defaultFiatCurrency
      })
    }

    if (quoteUnion.zeroExQuote) {
      return getZeroExQuoteOptions({
        quote: quoteUnion.zeroExQuote,
        fromNetwork,
        fromToken,
        toToken,
        spotPrices: spotPriceRegistry,
        defaultFiatCurrency
      })
    }

    return []
  }, [
    fromNetwork,
    fromToken,
    toToken,
    quoteUnion,
    spotPriceRegistry,
    defaultFiatCurrency
  ])

  const reset = useCallback(() => {
    setQuoteUnion(undefined)
    setQuoteErrorUnion(undefined)
    setIsFetchingQuote(false)
    setSwapFees(undefined)
    if (abortController) {
      abortController.abort()
    }
  }, [abortController])

  const onSelectQuoteOption = useCallback(
    (index: number) => {
      const option = quoteOptions[index]
      if (!option) {
        return
      }

      if (!toToken) {
        return
      }

      setToAmount(option.toAmount.format(6))
    },
    [quoteOptions, toToken]
  )

  const handleQuoteRefreshInternal = useCallback(
    async (overrides: SwapParamsOverrides) => {
      if (!fromAccount || !toAccountId) {
        return
      }

      const params = {
        fromAmount:
          overrides.fromAmount === undefined
            ? fromAmount
            : overrides.fromAmount,
        toAmount:
          overrides.toAmount === undefined ? toAmount : overrides.toAmount,
        fromToken: overrides.fromToken || fromToken,
        toToken: overrides.toToken || toToken,
        editingFromOrToAmount
      }

      if (params.fromAmount && !params.toAmount) {
        params.editingFromOrToAmount = 'from'
      } else if (params.toAmount && !params.fromAmount) {
        params.editingFromOrToAmount = 'to'
      }

      if (!params.fromToken || !params.toToken) {
        return
      }

      const fromAmountWrapped = new Amount(params.fromAmount)
      const toAmountWrapped = new Amount(params.toAmount)
      const isFromAmountEmpty =
        fromAmountWrapped.isZero() ||
        fromAmountWrapped.isNaN() ||
        fromAmountWrapped.isUndefined()
      const isToAmountEmpty =
        toAmountWrapped.isZero() ||
        toAmountWrapped.isNaN() ||
        toAmountWrapped.isUndefined()

      if (isFromAmountEmpty && isToAmountEmpty) {
        reset()
        return
      }

      const controller = new AbortController()
      setAbortController(controller)
      setIsFetchingQuote(true)

      let quoteResponse
      try {
        quoteResponse = await generateSwapQuote({
          fromAccountId: fromAccount.accountId,
          fromChainId: params.fromToken.chainId,
          fromAmount:
            params.editingFromOrToAmount === 'from' && params.fromAmount
              ? new Amount(params.fromAmount)
                  .multiplyByDecimals(params.fromToken.decimals)
                  .format()
              : '',
          fromToken: params.fromToken.contractAddress,
          toAccountId,
          toChainId: params.toToken.chainId,
          toAmount:
            params.editingFromOrToAmount === 'to' && params.toAmount
              ? new Amount(params.toAmount)
                  .multiplyByDecimals(params.toToken.decimals)
                  .format()
              : '',
          toToken: params.toToken.contractAddress,
          slippagePercentage: slippageTolerance,
          routePriority: BraveWallet.RoutePriority.kRecommended
        }).unwrap()
      } catch (e) {
        console.error(`generateSwapQuote failed: ${e}`)
      }

      if (controller.signal.aborted) {
        setIsFetchingQuote(false)
        setAbortController(undefined)
        return
      }

      if (quoteResponse?.error) {
        setQuoteErrorUnion(quoteResponse.error)
      }

      if (quoteResponse?.response) {
        setQuoteUnion(quoteResponse.response)

        if (quoteResponse.response.jupiterQuote) {
          if (params.editingFromOrToAmount === 'from') {
            setToAmount(
              getJupiterToAmount({
                quote: quoteResponse.response.jupiterQuote,
                toToken: params.toToken
              }).format(6)
            )
          } else {
            setFromAmount(
              getJupiterFromAmount({
                quote: quoteResponse.response.jupiterQuote,
                fromToken: params.fromToken
              }).format(6)
            )
          }
        }

        if (quoteResponse.response.zeroExQuote) {
          if (params.editingFromOrToAmount === 'from') {
            setToAmount(
              getZeroExToAmount({
                quote: quoteResponse.response.zeroExQuote,
                toToken: params.toToken
              }).format(6)
            )
          } else {
            setFromAmount(
              getZeroExFromAmount({
                quote: quoteResponse.response.zeroExQuote,
                fromToken: params.fromToken
              }).format(6)
            )
          }

          await zeroEx.checkAllowance(quoteResponse.response.zeroExQuote)
        }
      }

      if (quoteResponse?.fees) {
        setSwapFees(quoteResponse?.fees)
      }

      setIsFetchingQuote(false)
      setAbortController(undefined)
    },
    [
      editingFromOrToAmount,
      fromAccount,
      fromAmount,
      fromToken,
      toAmount,
      toToken,
      generateSwapQuote,
      slippageTolerance
    ]
  )

  const handleQuoteRefresh = useDebouncedCallback(
    async (overrides: SwapParamsOverrides) => {
      await handleQuoteRefreshInternal(overrides)
    },
    700
  )

  // Changing the From amount does the following:
  //  - Set the fromAmount field.
  //  - Clear the toAmount field.
  //  - Refresh quotes based on the new fromAmount, with debouncing.
  const handleOnSetFromAmount = useCallback(
    async (value: string) => {
      setFromAmount(value)
      setEditingFromOrToAmount('from')
      if (!value) {
        setToAmount('')
      }

      await handleQuoteRefresh({
        fromAmount: value,
        toAmount: ''
      })
    },
    [handleQuoteRefresh]
  )

  // Changing the To amount does the following:
  //  - Set the toAmount field.
  //  - Clear the fromAmount field.
  //  - Refresh quotes based on the new toAmount, with debouncing.
  const handleOnSetToAmount = useCallback(
    async (value: string) => {
      setToAmount(value)
      setEditingFromOrToAmount('to')
      if (!value) {
        setFromAmount('')
      }

      await handleQuoteRefresh({
        fromAmount: '',
        toAmount: value
      })
    },
    [handleQuoteRefresh]
  )

  const getAssetBalance = useCallback(
    (token: BraveWallet.BlockchainToken): Amount => {
      if (!fromAccount) {
        return Amount.zero()
      }

      return new Amount(
        getBalance(fromAccount.accountId, token, tokenBalancesRegistry)
      )
    },
    [tokenBalancesRegistry, fromAccount]
  )

  const fromAssetBalance = fromToken && getAssetBalance(fromToken)
  const nativeAssetBalance = nativeAsset && getAssetBalance(nativeAsset)

  const onClickFlipSwapTokens = useCallback(async () => {
    setFromToken(toToken)
    setToToken(fromToken)
    await handleOnSetFromAmount('')
  }, [toToken, fromToken, handleOnSetFromAmount])

  // Changing the To asset does the following:
  //  1. Update toToken right away.
  //  2. Reset toAmount.
  //  3. Reset the previously displayed quote, if any.
  //  4. Fetch new quotes using fromAmount and fromToken, if set. Do NOT use
  //     debouncing.
  //  5. Fetch spot price.
  const onSelectToToken = useCallback(
    async (token: BraveWallet.BlockchainToken) => {
      setEditingFromOrToAmount('from')
      setToToken(token)
      setSelectingFromOrTo(undefined)
      setToAmount('')

      reset()
      await handleQuoteRefreshInternal({
        toToken: token,
        toAmount: ''
      })
    },
    [handleQuoteRefreshInternal]
  )

  // Changing the From asset does the following:
  //  1. Reset both fromAmount and toAmount.
  //  2. Reset the previously displayed quote, if any.
  //  3. Refresh balance for the new fromToken.
  //  4. Refresh spot price.
  const onSelectFromToken = useCallback(
    async (
      token: BraveWallet.BlockchainToken,
      account?: BraveWallet.AccountInfo
    ) => {
      setEditingFromOrToAmount('from')
      setFromToken(token)
      setFromAccount(account)

      // TODO(onyb): remove this when we support cross-chain swaps
      if (token.coin !== toToken?.coin || token.chainId !== toToken?.chainId) {
        setToToken(undefined)
      }

      // TODO(onyb): change this when we support swapping to different address
      setToAccountId(account?.accountId)

      setSelectingFromOrTo(undefined)
      setFromAmount('')
      setToAmount('')
      reset()
    },
    [toToken, reset]
  )

  const onSetSelectedSwapAndSendOption = useCallback((value: string) => {
    if (value === 'to-account') {
      setToAnotherAddress('')
    }
    setSelectedSwapAndSendOption(value)
  }, [])

  const handleOnSetToAnotherAddress = useCallback((value: string) => {
    setToAnotherAddress(value)
  }, [])

  const onCheckUserConfirmedAddress = useCallback(
    (id: string, checked: boolean) => {
      setUserConfirmedAddress(checked)
    },
    []
  )

  // Memos
  const fiatValue = useMemo(() => {
    if (
      !fromAmount ||
      !fromToken ||
      !spotPriceRegistry ||
      !defaultFiatCurrency
    ) {
      return
    }

    const price = getTokenPriceAmountFromRegistry(spotPriceRegistry, fromToken)
    return new Amount(fromAmount).times(price).formatAsFiat(defaultFiatCurrency)
  }, [fromAmount, fromToken, spotPriceRegistry, defaultFiatCurrency])

  const gasEstimates: GasEstimate = useMemo(() => {
    // TODO(onyb): Setup getGasEstimate Methods
    return {
      gasFee: '0.0034',
      gasFeeGwei: '36',
      gasFeeFiat: '17.59',
      time: '1 min'
    }
  }, [])

  const feesWrapped = useMemo(() => {
    if (quoteOptions.length) {
      return quoteOptions[0].networkFee
    }

    return Amount.zero()
  }, [quoteOptions])

  const swapValidationError: SwapValidationErrorType | undefined =
    useMemo(() => {
      // No validation to perform when From and To amounts
      // are empty, since quote is not fetched.
      if (!fromAmount && !toAmount) {
        return
      }

      if (
        fromToken &&
        fromAmount &&
        hasDecimalsOverflow(fromAmount, fromToken)
      ) {
        return 'fromAmountDecimalsOverflow'
      }

      if (toToken && toAmount && hasDecimalsOverflow(toAmount, toToken)) {
        return 'toAmountDecimalsOverflow'
      }

      // No balance-based validations to perform when FROM/native balances
      // have not been fetched yet.
      if (!fromAssetBalance || !nativeAssetBalance) {
        return
      }

      if (!fromToken) {
        return
      }

      const fromAmountWeiWrapped = new Amount(fromAmount).multiplyByDecimals(
        fromToken.decimals
      )
      if (fromAmountWeiWrapped.gt(fromAssetBalance)) {
        return 'insufficientBalance'
      }

      if (feesWrapped.gt(nativeAssetBalance)) {
        return 'insufficientFundsForGas'
      }

      if (
        !fromToken.contractAddress &&
        fromAmountWeiWrapped.plus(feesWrapped).gt(fromAssetBalance)
      ) {
        return 'insufficientFundsForGas'
      }

      // 0x specific validations
      if (
        quoteUnion?.zeroExQuote &&
        fromToken.contractAddress &&
        !zeroEx.hasAllowance
      ) {
        return 'insufficientAllowance'
      }

      if (quoteErrorUnion?.zeroExError) {
        if (quoteErrorUnion.zeroExError.isInsufficientLiquidity) {
          return 'insufficientLiquidity'
        }

        return 'unknownError'
      }

      // Jupiter specific validations
      if (quoteErrorUnion?.jupiterError) {
        if (quoteErrorUnion.jupiterError.isInsufficientLiquidity) {
          return 'insufficientLiquidity'
        }

        return 'unknownError'
      }

      if (quoteUnion?.jupiterQuote?.routePlan.length === 0) {
        return 'insufficientLiquidity'
      }

      return undefined
    }, [
      fromToken,
      fromAmount,
      toToken,
      toAmount,
      feesWrapped,
      zeroEx,
      fromAssetBalance,
      nativeAssetBalance
    ])

  const onSubmit = useCallback(async () => {
    if (!quoteUnion) {
      return
    }

    setIsFetchingQuote(true)

    if (quoteUnion.zeroExQuote) {
      if (zeroEx.hasAllowance) {
        const error = await zeroEx.exchange()
        if (error) {
          setQuoteErrorUnion(error)
        } else {
          setFromAmount('')
          setToAmount('')
          reset()
        }
      } else {
        await zeroEx.approve(quoteUnion.zeroExQuote)
      }
    }

    if (quoteUnion.jupiterQuote) {
      const error = await jupiter.exchange(quoteUnion.jupiterQuote)
      if (error) {
        setQuoteErrorUnion(error)
      } else {
        setFromAmount('')
        setToAmount('')
        reset()
      }
    }

    setIsFetchingQuote(false)
  }, [quoteUnion, zeroEx, jupiter, reset])

  const submitButtonText = useMemo(() => {
    if (!fromToken || !fromNetwork) {
      return getLocale('braveSwapReviewOrder')
    }

    if (swapValidationError === 'insufficientBalance') {
      return getLocale('braveSwapInsufficientBalance').replace(
        '$1',
        fromToken.symbol
      )
    }

    if (swapValidationError === 'insufficientFundsForGas') {
      return getLocale('braveSwapInsufficientBalance').replace(
        '$1',
        fromNetwork.symbol
      )
    }

    if (
      fromNetwork.coin === BraveWallet.CoinType.ETH &&
      swapValidationError === 'insufficientAllowance'
    ) {
      return getLocale('braveSwapApproveToken').replace('$1', fromToken.symbol)
    }

    if (swapValidationError === 'insufficientLiquidity') {
      return getLocale('braveSwapInsufficientLiquidity')
    }

    return getLocale('braveSwapReviewOrder')
  }, [fromToken, fromNetwork, swapValidationError, getLocale])

  const isSubmitButtonDisabled = useMemo(() => {
    return (
      !fromNetwork ||
      !fromAccount ||
      !networkSupportsAccount(fromNetwork, fromAccount.accountId) ||
      !toNetwork ||
      // Prevent creating a swap transaction with stale parameters if fetching
      // of a new quote is in progress.
      isFetchingQuote ||
      // If quote is not set, there's nothing to create the swap with, so Swap
      // button must be disabled.
      quoteUnion === undefined ||
      // FROM/TO assets may be undefined during initialization of the swap
      // assets list.
      fromToken === undefined ||
      toToken === undefined ||
      // Amounts must be defined by the user, or populated from the swap quote,
      // for creating a transaction.
      new Amount(toAmount).isUndefined() ||
      new Amount(toAmount).isZero() ||
      new Amount(fromAmount).isUndefined() ||
      new Amount(fromAmount).isZero() ||
      // Disable Swap button if native asset balance has not been fetched yet,
      // to ensure insufficientFundsForGas error (if applicable) is accurate.
      nativeAssetBalance === undefined ||
      // Disable Swap button if FROM asset balance has not been fetched yet,
      // to ensure insufficientBalance error (if applicable) is accurate.
      fromAssetBalance === undefined ||
      // Unless the validation error is insufficientAllowance, in which case
      // the transaction is an ERC20Approve, Swap button must be disabled.
      (swapValidationError &&
        fromNetwork.coin === BraveWallet.CoinType.ETH &&
        swapValidationError !== 'insufficientAllowance') ||
      (swapValidationError && fromNetwork.coin === BraveWallet.CoinType.SOL)
    )
  }, [
    fromNetwork,
    fromAccount,
    toNetwork,
    isFetchingQuote,
    quoteUnion,
    fromToken,
    toToken,
    toAmount,
    fromAmount,
    nativeAssetBalance,
    fromAssetBalance,
    swapValidationError
  ])

  useEffect(() => {
    const interval = setInterval(async () => {
      await handleQuoteRefresh({})
    }, 10000)
    return () => {
      clearInterval(interval)
    }
  }, [handleQuoteRefresh])

  return {
    fromAccount,
    fromToken,
    fromAmount,
    fromNetwork,
    toNetwork,
    toToken,
    toAmount,
    fromAssetBalance: fromAssetBalance || Amount.zero(),
    fiatValue,
    isFetchingQuote,
    quoteOptions,
    selectedQuoteOptionIndex: 0,
    selectingFromOrTo,
    swapAndSendSelected,
    selectedSwapAndSendOption,
    selectedSwapSendAccount,
    toAnotherAddress,
    userConfirmedAddress,
    selectedGasFeeOption,
    slippageTolerance,
    useDirectRoute,
    gasEstimates,
    swapFees,
    onSelectFromToken,
    onSelectToToken,
    onSelectQuoteOption,
    setSelectingFromOrTo,
    handleOnSetFromAmount,
    handleOnSetToAmount,
    onClickFlipSwapTokens,
    setSwapAndSendSelected,
    handleOnSetToAnotherAddress,
    onCheckUserConfirmedAddress,
    onSetSelectedSwapAndSendOption,
    setSelectedSwapSendAccount,
    setSelectedGasFeeOption,
    setSlippageTolerance,
    setUseDirectRoute,
    onSubmit,
    submitButtonText,
    isSubmitButtonDisabled,
    swapValidationError,
    spotPrices: spotPriceRegistry,
    tokenBalancesRegistry,
    isLoadingBalances
  }
}
export default useSwap
