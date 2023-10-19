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
  useBalancesFetcher
} from '../../../../common/hooks/use-balances-fetcher'

// Types and constants
import {
  QuoteOption,
  GasFeeOption,
  GasEstimate,
  SwapParams,
  SwapValidationErrorType
} from '../constants/types'
import { BraveWallet } from '../../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import Amount from '../../../../utils/amount'
import { getPriceIdForToken } from '../../../../utils/api-utils'
// FIXME(onyb): move makeNetworkAsset to utils/assets-utils
import { makeNetworkAsset } from '../../../../options/asset-options'
import { getTokenPriceAmountFromRegistry } from '../../../../utils/pricing-utils'
import { getBalance } from '../../../../utils/balance-utils'

// Queries
import {
  useGetSelectedChainQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery
} from '../../../../common/slices/api.slice'
import {
  useGetCombinedTokensListQuery,
  useSelectedAccountQuery
} from '../../../../common/slices/api.slice.extra'
import {
  querySubscriptionOptions60s
} from '../../../../common/slices/constants'
import {
  AccountInfoEntity
} from '../../../../common/slices/entities/account-info.entity'

const hasDecimalsOverflow = (amount: string, asset?: BraveWallet.BlockchainToken) => {
  if (!asset) {
    return false
  }

  const amountBaseWrapped = new Amount(amount).multiplyByDecimals(asset.decimals)
  if (!amountBaseWrapped.value) {
    return false
  }

  const decimalPlaces = amountBaseWrapped.value.decimalPlaces()
  return decimalPlaces !== null && decimalPlaces > 0
}

export const useSwap = () => {
  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAccount } = useSelectedAccountQuery()
  const { data: assetsList } = useGetCombinedTokensListQuery()
  // FIXME(onyb): what happens when defaultFiatCurrency is empty
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const nativeAsset = useMemo(() => makeNetworkAsset(selectedNetwork), [selectedNetwork])

  // State
  const [fromToken, setFromToken] = useState<BraveWallet.BlockchainToken | undefined>(undefined)
  const [toToken, setToToken] = useState<BraveWallet.BlockchainToken | undefined>(undefined)
  const [fromAmount, setFromAmount] = useState<string>('')
  const [toAmount, setToAmount] = useState<string>('')
  const [selectingFromOrTo, setSelectingFromOrTo] = useState<'from' | 'to' | undefined>(undefined)
  const [selectedQuoteOptionIndex, setSelectedQuoteOptionIndex] = useState<number>(0)
  const [selectedSwapAndSendOption, setSelectedSwapAndSendOption] = useState<string>(
    SwapAndSendOptions[0].name
  )
  const [swapAndSendSelected, setSwapAndSendSelected] = useState<boolean>(false)
  const [toAnotherAddress, setToAnotherAddress] = useState<string>('')
  const [userConfirmedAddress, setUserConfirmedAddress] = useState<boolean>(false)
  const [selectedSwapSendAccount, setSelectedSwapSendAccount] = useState<
    AccountInfoEntity | undefined
  >(selectedAccount)
  const [useDirectRoute, setUseDirectRoute] = useState<boolean>(false)
  const [slippageTolerance, setSlippageTolerance] = useState<string>('0.5')
  const [selectedGasFeeOption, setSelectedGasFeeOption] = useState<GasFeeOption>(gasFeeOptions[1])

  const { data: tokenBalancesRegistry } = useBalancesFetcher(
    selectedNetwork && selectedAccount
      ? {
          networks: [selectedNetwork],
          accounts: [selectedAccount]
        }
      : skipToken
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

  const resetSelectedAssets = useCallback(() => {
    setFromToken(nativeAsset)
    setToToken(undefined)
    setFromAmount('')
    setToAmount('')
  }, [nativeAsset])

  // Reset FROM asset when network changes
  useEffect(resetSelectedAssets, [resetSelectedAssets])

  const jupiter = useJupiter({
    fromToken,
    toToken,
    fromAmount,
    toAmount: '',
    slippageTolerance,
    fromAddress: selectedAccount?.address,
    spotPrices: spotPriceRegistry
  })
  const zeroEx = useZeroEx({
    fromAmount,
    toAmount: '',
    fromToken,
    toToken,
    slippageTolerance,
    fromAddress: selectedAccount?.address,
    spotPrices: spotPriceRegistry
  })

  const quoteOptions: QuoteOption[] = useMemo(() => {
    if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
      return jupiter.quoteOptions
    }

    return zeroEx.quoteOptions
  }, [selectedNetwork?.coin, jupiter.quoteOptions, zeroEx.quoteOptions])

  const onSelectQuoteOption = useCallback(
    (index: number) => {
      const option = quoteOptions[index]
      if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
        if (jupiter.quote && jupiter.quote.routes.length > index && toToken) {
          const route = jupiter.quote.routes[index]
          jupiter.setSelectedRoute(route)
          setToAmount(option.toAmount.format(6))
        }
      } else if (selectedNetwork?.coin === BraveWallet.CoinType.ETH) {
        if (zeroEx.quote && toToken) {
          setToAmount(option.toAmount.format(6))
        }
      }

      setSelectedQuoteOptionIndex(index)
    },
    [quoteOptions, selectedNetwork?.coin, jupiter, toToken, zeroEx.quote]
  )

  // Methods
  const getNetworkAssetsList = useCallback(
    (networkInfo: BraveWallet.NetworkInfo) => {
      return assetsList.filter(
        asset =>
          asset.coin === networkInfo.coin &&
          asset.chainId === networkInfo.chainId &&
          !asset.isNft &&
          !asset.isErc1155 &&
          !asset.isErc721
      )
    },
    [assetsList]
  )

  const handleJupiterQuoteRefreshInternal = useCallback(
    async (overrides: Partial<SwapParams>) => {
      const quote = await jupiter.refresh(overrides)
      if (!quote) {
        return
      }

      if (overrides.fromAmount === '') {
        const token = overrides.fromToken || fromToken
        if (token && quote.routes.length > 0) {
          setFromAmount(
            new Amount(quote.routes[0].inAmount.toString())
              .divideByDecimals(token.decimals)
              .format(6)
          )
        }
      }

      if (overrides.toAmount === '') {
        const token = overrides.toToken || toToken
        if (token && quote.routes.length > 0) {
          setToAmount(
            new Amount(quote.routes[0].outAmount.toString())
              .divideByDecimals(token.decimals)
              .format(6)
          )
        }
      }
    },
    [jupiter, toToken, fromToken]
  )
  const handleJupiterQuoteRefresh = useDebouncedCallback(async (overrides: Partial<SwapParams>) => {
    await handleJupiterQuoteRefreshInternal(overrides)
  }, 700)

  const handleZeroExQuoteRefreshInternal = useCallback(
    async (overrides: Partial<SwapParams>) => {
      const quote = await zeroEx.refresh(overrides)
      if (!quote) {
        return
      }

      if (overrides.fromAmount === '') {
        const token = overrides.fromToken || fromToken
        if (token) {
          setFromAmount(new Amount(quote.sellAmount).divideByDecimals(token.decimals).format(6))
        }
      }

      if (overrides.toAmount === '') {
        const token = overrides.toToken || toToken
        if (token) {
          setToAmount(new Amount(quote.buyAmount).divideByDecimals(token.decimals).format(6))
        }
      }
    },
    [zeroEx, toToken, fromToken]
  )

  const handleZeroExQuoteRefresh = useDebouncedCallback(async (overrides: Partial<SwapParams>) => {
    await handleZeroExQuoteRefreshInternal(overrides)
  }, 700)

  // Changing the From amount does the following:
  //  - Set the fromAmount field.
  //  - Clear the toAmount field.
  //  - Refresh quotes based on the new fromAmount, with debouncing.
  const handleOnSetFromAmount = useCallback(
    async (value: string) => {
      setFromAmount(value)
      if (!value) {
        setToAmount('')
      }

      if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
        await handleJupiterQuoteRefresh({
          fromAmount: value,
          toAmount: ''
        })
      } else {
        await handleZeroExQuoteRefresh({
          fromAmount: value,
          toAmount: ''
        })
      }
    },
    [selectedNetwork?.coin, handleJupiterQuoteRefresh, handleZeroExQuoteRefresh]
  )

  // Changing the To amount does the following:
  //  - Set the toAmount field.
  //  - Clear the fromAmount field.
  //  - Refresh quotes based on the new toAmount, with debouncing.
  const handleOnSetToAmount = useCallback(
    async (value: string) => {
      setToAmount(value)
      if (!value) {
        setFromAmount('')
      }

      if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
        await handleJupiterQuoteRefresh({
          fromAmount: '',
          toAmount: value
        })
      } else {
        await handleZeroExQuoteRefresh({
          fromAmount: '',
          toAmount: value
        })
      }
    },
    [selectedNetwork?.coin, handleZeroExQuoteRefresh, handleJupiterQuoteRefresh]
  )

  const getAssetBalance = useCallback(
    (token: BraveWallet.BlockchainToken): Amount => {
      if (!selectedAccount) {
        return Amount.zero()
      }

      return new Amount(
        getBalance(selectedAccount.accountId, token, tokenBalancesRegistry)
      )
    },
    [tokenBalancesRegistry, selectedAccount]
  )

  const fromAssetBalance = fromToken && getAssetBalance(fromToken)
  const nativeAssetBalance = nativeAsset && getAssetBalance(nativeAsset)

  const onClickFlipSwapTokens = useCallback(async () => {
    setFromToken(toToken)
    setToToken(fromToken)
    await handleOnSetFromAmount('')
  }, [
    toToken,
    fromToken,
    handleOnSetFromAmount,
    selectedAccount,
    getAssetBalance,
    selectedNetwork
  ])

  // Changing the To asset does the following:
  //  1. Update toToken right away.
  //  2. Reset toAmount.
  //  3. Reset the previously displayed quote, if any.
  //  4. Fetch new quotes using fromAmount and fromToken, if set. Do NOT use
  //     debouncing.
  //  5. Fetch spot price.
  const onSelectToToken = useCallback(
    async (token: BraveWallet.BlockchainToken) => {
      setToToken(token)
      setSelectingFromOrTo(undefined)
      setToAmount('')

      if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
        await jupiter.reset()
        await handleJupiterQuoteRefreshInternal({
          toToken: token,
          toAmount: ''
        })
      } else if (selectedNetwork?.coin === BraveWallet.CoinType.ETH) {
        await zeroEx.reset()
        await handleZeroExQuoteRefreshInternal({
          toToken: token,
          toAmount: ''
        })
      }
    },
    [
      selectedNetwork?.coin,
      jupiter,
      handleJupiterQuoteRefreshInternal,
      zeroEx,
      handleZeroExQuoteRefreshInternal
    ]
  )

  // Changing the From asset does the following:
  //  1. Reset both fromAmount and toAmount.
  //  2. Reset the previously displayed quote, if any.
  //  3. Refresh balance for the new fromToken.
  //  4. Refresh spot price.
  const onSelectFromToken = useCallback(
    async (token: BraveWallet.BlockchainToken) => {
      setFromToken(token)
      setSelectingFromOrTo(undefined)
      setFromAmount('')
      setToAmount('')

      if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
        await jupiter.reset()
      } else if (selectedNetwork?.coin === BraveWallet.CoinType.ETH) {
        await zeroEx.reset()
      }
    },
    [
      selectedAccount,
      selectedNetwork,
      zeroEx,
      jupiter
    ]
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

  const onCheckUserConfirmedAddress = useCallback((id: string, checked: boolean) => {
    setUserConfirmedAddress(checked)
  }, [])

  // Memos
  const fiatValue = useMemo(() => {
    if (!fromAmount || !fromToken || !spotPriceRegistry) {
      return
    }

    const price = getTokenPriceAmountFromRegistry(spotPriceRegistry, fromToken)
    return new Amount(fromAmount).times(price).formatAsFiat(defaultFiatCurrency)
  }, [spotPriceRegistry, fromAmount, defaultFiatCurrency])

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
    if (selectedNetwork?.coin === BraveWallet.CoinType.ETH) {
      if (zeroEx.networkFee.isUndefined()) {
        return Amount.zero()
      } else {
        return zeroEx.networkFee
      }
    } else if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
      if (jupiter.networkFee.isUndefined()) {
        return Amount.zero()
      } else {
        return jupiter.networkFee
      }
    }

    return Amount.zero()
  }, [selectedNetwork?.coin, zeroEx.networkFee, jupiter.networkFee])

  const swapValidationError: SwapValidationErrorType | undefined = useMemo(() => {
    // No validation to perform when From and To amounts
    // are empty, since quote is not fetched.
    if (!fromAmount && !toAmount) {
      return
    }

    if (fromToken && fromAmount && hasDecimalsOverflow(fromAmount, fromToken)) {
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

    const fromAmountWeiWrapped = new Amount(fromAmount).multiplyByDecimals(fromToken.decimals)
    if (fromAmountWeiWrapped.gt(fromAssetBalance)) {
      return 'insufficientBalance'
    }

    if (feesWrapped.gt(nativeAssetBalance)) {
      return 'insufficientFundsForGas'
    }

    if (
      fromToken.symbol === selectedNetwork?.symbol &&
      fromAmountWeiWrapped.plus(feesWrapped).gt(fromAssetBalance)
    ) {
      return 'insufficientFundsForGas'
    }

    // 0x specific validations
    if (selectedNetwork?.coin === BraveWallet.CoinType.ETH) {
      if (fromToken.contractAddress && !zeroEx.hasAllowance) {
        return 'insufficientAllowance'
      }

      if (zeroEx.error === undefined) {
        return
      }

      if (zeroEx.error.isInsufficientLiquidity) {
        return 'insufficientLiquidity'
      }

      return 'unknownError'
    }

    // Jupiter specific validations
    if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
      if (jupiter.error?.isInsufficientLiquidity || jupiter.quote?.routes?.length === 0) {
        return 'insufficientLiquidity'
      }

      if (jupiter.error === undefined) {
        return
      }

      return 'unknownError'
    }

    return undefined
  }, [
    fromToken,
    fromAmount,
    toToken,
    toAmount,
    selectedNetwork,
    feesWrapped,
    zeroEx,
    jupiter,
    fromAssetBalance,
    nativeAssetBalance
  ])

  const onSubmit = useCallback(async () => {
    if (selectedNetwork?.coin === BraveWallet.CoinType.ETH) {
      if (zeroEx.hasAllowance) {
        await zeroEx.exchange({}, async function () {
          setFromAmount('')
          setToAmount('')
        })
      } else {
        await zeroEx.approve()
      }
    } else if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
      await jupiter.exchange(async function () {
        setFromAmount('')
        setToAmount('')
      })
    }
  }, [selectedNetwork?.coin, zeroEx, jupiter])

  const submitButtonText = useMemo(() => {
    if (!selectedAccount) {
      return getLocale('braveSwapConnectWallet')
    }

    if (!fromToken) {
      return getLocale('braveSwapReviewOrder')
    }

    if (swapValidationError === 'insufficientBalance') {
      return getLocale('braveSwapInsufficientBalance').replace('$1', fromToken.symbol)
    }

    if (swapValidationError === 'insufficientFundsForGas') {
      return getLocale('braveSwapInsufficientBalance').replace('$1', selectedNetwork?.symbol || '')
    }

    if (selectedNetwork?.coin === BraveWallet.CoinType.ETH) {
      if (swapValidationError === 'insufficientAllowance') {
        return getLocale('braveSwapApproveToken').replace('$1', fromToken.symbol)
      }
    }

    if (swapValidationError === 'insufficientLiquidity') {
      return getLocale('braveSwapInsufficientLiquidity')
    }

    return getLocale('braveSwapReviewOrder')
  }, [selectedAccount, fromToken, swapValidationError, selectedNetwork, getLocale])

  const isSubmitButtonDisabled = useMemo(() => {
    return (
      // Prevent creating a swap transaction with stale parameters if fetching
      // of a new quote is in progress.
      zeroEx.loading ||
      jupiter.loading ||
      // If 0x swap quote is empty, there's nothing to create the swap
      // transaction with, so Swap button must be disabled.
      (selectedNetwork?.coin === BraveWallet.CoinType.ETH && zeroEx.quote === undefined) ||
      // If Jupiter quote is empty, there's nothing to create the swap
      // transaction with, so Swap button must be disabled.
      (selectedNetwork?.coin === BraveWallet.CoinType.SOL && jupiter.quote === undefined) ||
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
        selectedNetwork?.coin === BraveWallet.CoinType.ETH &&
        swapValidationError !== 'insufficientAllowance') ||
      (swapValidationError && selectedNetwork?.coin === BraveWallet.CoinType.SOL)
    )
  }, [
    zeroEx.loading,
    jupiter.loading,
    zeroEx.quote,
    jupiter.quote,
    selectedNetwork?.coin,
    fromToken,
    toToken,
    fromAmount,
    toAmount,
    nativeAssetBalance,
    fromAssetBalance,
    swapValidationError
  ])

  useEffect(() => {
    const interval = setInterval(async () => {
      if (selectedNetwork?.coin === BraveWallet.CoinType.SOL) {
        await handleJupiterQuoteRefresh({})
      } else {
        await handleZeroExQuoteRefresh({})
      }
    }, 10000)
    return () => {
      clearInterval(interval)
    }
  }, [selectedNetwork?.coin, handleJupiterQuoteRefresh, handleZeroExQuoteRefresh])

  return {
    fromToken,
    toToken,
    fromAmount,
    toAmount,
    fromAssetBalance: fromAssetBalance || Amount.zero(),
    fiatValue,
    isFetchingQuote: zeroEx.loading || jupiter.loading,
    quoteOptions,
    selectedQuoteOptionIndex,
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
    onSelectFromToken,
    onSelectToToken,
    getCachedAssetBalance: getAssetBalance,
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
    getNetworkAssetsList,
    spotPrices: spotPriceRegistry
  }
}
export default useSwap
