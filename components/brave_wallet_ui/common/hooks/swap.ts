// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SimpleActionCreator } from 'redux-act'

// Constants
import {
  BraveWallet,
  ExpirationPresetObjectType,
  OrderTypes,
  SlippagePresetObjectType,
  SwapErrorResponse,
  SwapValidationErrorType,
  ToOrFromType,
  WalletAccountType,
  ApproveERC20Params
} from '../../constants/types'
import { SwapParamsPayloadType } from '../constants/action_types'
import { MAX_UINT256 } from '../constants/magics'

// Options
import { SlippagePresetOptions } from '../../options/slippage-preset-options'
import { ExpirationPresetOptions } from '../../options/expiration-preset-options'
import { makeNetworkAsset } from '../../options/asset-options'

// Utils
import Amount from '../../utils/amount'
import { debounce } from '../../../common/debounce'

// Hooks
import useBalance from './balance'

const SWAP_VALIDATION_ERROR_CODE = 100

export default function useSwap (
  selectedAccount: WalletAccountType,
  selectedNetwork: BraveWallet.NetworkInfo,
  swapAssetOptions: BraveWallet.BlockchainToken[],
  fetchSwapQuote: SimpleActionCreator<SwapParamsPayloadType>,
  getERC20Allowance: (contractAddress: string, ownerAddress: string, spenderAddress: string) => Promise<string>,
  approveERC20Allowance: SimpleActionCreator<ApproveERC20Params>,
  isSwapSupported: (network: BraveWallet.NetworkInfo) => Promise<boolean>,
  quote?: BraveWallet.SwapResponse,
  rawError?: SwapErrorResponse
) {
  const [exchangeRate, setExchangeRate] = React.useState('')
  const [fromAmount, setFromAmount] = React.useState('')
  const [fromAsset, setFromAsset] = React.useState<BraveWallet.BlockchainToken | undefined>(swapAssetOptions[0])
  const [orderExpiration, setOrderExpiration] = React.useState<ExpirationPresetObjectType>(ExpirationPresetOptions[0])
  const [orderType, setOrderType] = React.useState<OrderTypes>('market')
  const [slippageTolerance, setSlippageTolerance] = React.useState<SlippagePresetObjectType>(SlippagePresetOptions[0])
  const [customSlippageTolerance, setCustomSlippageTolerance] = React.useState<string>('')
  const [toAmount, setToAmount] = React.useState('')
  const [toAsset, setToAsset] = React.useState<BraveWallet.BlockchainToken | undefined>(swapAssetOptions[1])
  const [filteredAssetList, setFilteredAssetList] = React.useState<BraveWallet.BlockchainToken[]>(swapAssetOptions)
  const [swapToOrFrom, setSwapToOrFrom] = React.useState<ToOrFromType>('from')
  const [allowance, setAllowance] = React.useState<string | undefined>(undefined)
  const [isLoading, setIsLoading] = React.useState<boolean>(false)
  const [isSupported, setIsSupported] = React.useState<boolean>(false)

  const nativeAsset = React.useMemo(
    () => makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )

  React.useEffect(() => {
    setFromAsset(swapAssetOptions[0])
    setToAsset(swapAssetOptions[1])
  }, [swapAssetOptions])

  React.useEffect(() => {
    isSwapSupported(selectedNetwork).then(
      (supported) => setIsSupported(supported)
    )
  }, [selectedNetwork])

  React.useEffect(() => {
    if (!fromAsset) {
      return
    }

    if (!fromAsset.isErc20) {
      setAllowance(undefined)
      return
    }

    if (!quote) {
      setAllowance(undefined)
      return
    }

    const { allowanceTarget } = quote

    getERC20Allowance(fromAsset.contractAddress, selectedAccount.address, allowanceTarget)
      .then(value => setAllowance(value))
      .catch(e => console.log(e))
  }, [fromAsset, quote, selectedAccount])

  const getBalance = useBalance(selectedNetwork)
  const fromAssetBalance = getBalance(selectedAccount, fromAsset)
  const nativeAssetBalance = getBalance(selectedAccount, nativeAsset)

  const feesWrapped = React.useMemo(() => {
    if (!quote) {
      return new Amount('0')
    }

    // NOTE: Swap will eventually use EIP-1559 gas fields, but we rely on
    // gasPrice as a fee-ceiling for validation of inputs.
    const { gasPrice, gas } = quote
    const gasPriceWrapped = new Amount(gasPrice)
    const gasWrapped = new Amount(gas)
    return gasPriceWrapped.times(gasWrapped)
  }, [quote])

  const hasDecimalsOverflow = React.useCallback(
    (amount: string, asset?: BraveWallet.BlockchainToken) => {
      if (!asset) {
        return false
      }

      const amountBaseWrapped = new Amount(amount).multiplyByDecimals(asset.decimals)
      return amountBaseWrapped.value && amountBaseWrapped.value.decimalPlaces() > 0
    }, []
  )

  const swapValidationError: SwapValidationErrorType | undefined = React.useMemo(() => {
    if (!fromAsset || !toAsset) {
      return
    }

    // No validation to perform when From and To amounts are empty, since quote
    // is not fetched.
    if (!fromAmount && !toAmount) {
      return
    }

    if (hasDecimalsOverflow(fromAmount, fromAsset)) {
      return 'fromAmountDecimalsOverflow'
    }

    if (hasDecimalsOverflow(toAmount, toAsset)) {
      return 'toAmountDecimalsOverflow'
    }

    const fromAmountWeiWrapped = new Amount(fromAmount)
      .multiplyByDecimals(fromAsset.decimals)

    if (fromAmountWeiWrapped.gt(fromAssetBalance)) {
      return 'insufficientBalance'
    }

    if (feesWrapped.gt(nativeAssetBalance)) {
      return 'insufficientFundsForGas'
    }

    if (fromAsset.symbol === selectedNetwork.symbol && fromAmountWeiWrapped.plus(feesWrapped).gt(fromAssetBalance)) {
      return 'insufficientFundsForGas'
    }

    if (allowance !== undefined && new Amount(allowance).lt(fromAmountWeiWrapped)) {
      return 'insufficientAllowance'
    }

    if (rawError === undefined) {
      return
    }

    const { code, validationErrors } = rawError
    switch (code) {
      case SWAP_VALIDATION_ERROR_CODE:
        if (validationErrors?.find(err => err.reason === 'INSUFFICIENT_ASSET_LIQUIDITY')) {
          return 'insufficientLiquidity'
        }
        break

      default:
        return 'unknownError'
    }

    return undefined
  }, [
    fromAsset,
    fromAmount,
    toAsset,
    toAmount,
    fromAssetBalance,
    nativeAssetBalance,
    feesWrapped,
    rawError,
    allowance
  ])

  /**
   * React effect to extract fields from the swap quote and write the relevant
   * fields to the state.
   */
  React.useEffect(() => {
    if (!quote) {
      setFromAmount('')
      setToAmount('')
      setExchangeRate('')
      return
    }

    if (!fromAsset || !toAsset) {
      return
    }

    const {
      buyAmount,
      sellAmount,
      price,

      // The two fields below use the underlying native asset for other EVM
      // compatible networks, even though they are  called *ToEthRate.
      buyTokenToEthRate,
      sellTokenToEthRate
    } = quote

    setFromAmount(new Amount(sellAmount)
      .divideByDecimals(fromAsset.decimals)
      .format())

    setToAmount(new Amount(buyAmount)
      .divideByDecimals(toAsset.decimals)
      .format())

    /**
     * Price computation block
     *
     * Price returned by 0x is inverted when the user enters the amount in the
     * To field. In such a case, we use an approximate price computed from ETH
     * rates.
     *
     * Example:
     *   let x = DAI-ETH rate
     *   let y = BAT-ETH rate
     *   => DAI-BAT rate = x / y
     *
     * If the approximate price is numerically close to the quoted price, we
     * consider the latter. This is typically when the user modifies the amount
     * in the From field.
     */
    const priceWrapped = new Amount(price)
    const approxPriceWrapped = new Amount(buyTokenToEthRate).div(sellTokenToEthRate)
    let bestEstimatePriceWrapped
    if (approxPriceWrapped.div(priceWrapped).eq(new Amount(1))) {
      bestEstimatePriceWrapped = priceWrapped
    } else if (priceWrapped.div(approxPriceWrapped).eq(new Amount(1))) {
      bestEstimatePriceWrapped = priceWrapped
    } else {
      bestEstimatePriceWrapped = approxPriceWrapped
    }

    const bestEstimatePrice = bestEstimatePriceWrapped.format(6)
    setExchangeRate(bestEstimatePrice)
  }, [quote])

  /**
   * onSwapParamsChange() is triggered whenever a change in the swap fields
   * requires fetching the swap quote.
   *
   * @param {Object} overrides Override fields from the React state.
   * @param {Object} state     State fields used to prevent recreating the
   *                           function on change.
   */
  const onSwapParamsChange = React.useCallback((
    overrides: {
      toOrFrom: ToOrFromType
      fromAsset?: BraveWallet.BlockchainToken
      toAsset?: BraveWallet.BlockchainToken
      amount?: string
      slippageTolerance?: SlippagePresetObjectType
    },
    state: {
      fromAmount: string
      toAmount: string
    },
    full: boolean = false
  ) => {
    /**
     * STEP 1: Get the fromAsset/toAsset pairs to actually use for fetching
     * the swap quote.
     *
     * Special cases:
     *   - Typically, either fromAsset or toAsset is overridden but not both,
     *     except when the asset pair is flipped.
     */
    let fromAssetNext = overrides.fromAsset ?? fromAsset
    let toAssetNext = overrides.toAsset ?? toAsset

    if (!fromAssetNext || !toAssetNext) {
      return
    }

    let fromAmountWeiWrapped
    let toAmountWeiWrapped

    /**
     * STEP 2: Get the amount (in base units) to associate with the From field
     * for fetching the swap quote.
     *
     * This value is passed as `sellAmount` to the 0x API.
     *
     * Special cases:
     *   - If the change that triggered onSwapParamsChange() does not concern
     *     the From field, the amount is considered `undefined`.
     */
    if (overrides.toOrFrom === 'from') {
      if (hasDecimalsOverflow(
        overrides.amount ?? state.fromAmount,
        fromAssetNext
      )) {
        return
      }

      fromAmountWeiWrapped = new Amount(overrides.amount ?? state.fromAmount)
        .multiplyByDecimals(fromAssetNext.decimals)
    }

    /**
     * STEP 3: Get the amount (in base units) to associate with the To field
     * for fetching the swap quote.
     *
     * This value is passed as `buyAmount` to the 0x API.
     *
     * Special cases:
     *   - If the change that triggered onSwapParamsChange() does not concern
     *     the To field, the amount is considered `undefined`.
     *
     *   - If the change that triggered onSwapParamsChange() concerns the To
     *     field, AND the `toAsset` is overridden, we fetch the quote based
     *     on the amount in the From field. The amount in the To field is
     *     considered `undefined` in this case.
     */
    if (overrides.toOrFrom === 'to') {
      if (hasDecimalsOverflow(
        overrides.amount ?? state.toAmount,
        toAssetNext
      )) {
        return
      }

      if (overrides.toAsset === undefined) {
        toAmountWeiWrapped = new Amount(overrides.amount ?? state.toAmount)
          .multiplyByDecimals(toAssetNext.decimals)
      } else {
        fromAmountWeiWrapped = new Amount(state.fromAmount)
          .multiplyByDecimals(fromAssetNext.decimals)
      }
    }

    /**
     * STEP 4: Reset amount fields that effectively have zero value.
     *
     * The following block makes it impossible to enter 0-ish amount
     * values.
     */
    if (toAmountWeiWrapped?.isUndefined() || toAmountWeiWrapped?.isZero()) {
      setToAmount('')
      return
    }

    if (fromAmountWeiWrapped?.isUndefined() || fromAmountWeiWrapped?.isZero()) {
      setFromAmount('')
      return
    }

    /**
     * STEP 5: Fetch the swap quote asynchronously.
     */
    setIsLoading(true)
    fetchSwapQuote({
      fromAsset: fromAssetNext,
      fromAssetAmount: fromAmountWeiWrapped?.format(),
      toAsset: toAssetNext,
      toAssetAmount: toAmountWeiWrapped?.format(),
      accountAddress: selectedAccount.address,
      slippageTolerance: overrides.slippageTolerance ?? slippageTolerance,
      networkChainId: selectedNetwork.chainId,
      full
    })
  }, [selectedAccount, selectedNetwork, fromAsset, toAsset])

  // Set isLoading to false as soon as:
  //  - swap quote has been fetched.
  //  - error from 0x API is available in rawError.
  React.useEffect(() => setIsLoading(false), [quote, rawError])

  const onSwapQuoteRefresh = () => {
    const customSlippage = {
      id: 4,
      slippage: Number(customSlippageTolerance)
    }
    onSwapParamsChange(
      { toOrFrom: 'from', slippageTolerance: customSlippageTolerance ? customSlippage : slippageTolerance },
      { fromAmount, toAmount }
    )
  }

  /**
   * onSwapParamsChangeDebounced is a debounced function which delays calling
   * the onSwapParamsChange() function until after the stated wait time in
   * milliseconds have passed since the last time this debounced function was
   * called.
   *
   * CAUTION: This function is typically used to debounce changes in the amount
   * fields, and must NOT be recreated on component renders due to change in
   * amount values.
   */
  const onSwapParamsChangeDebounced = React.useCallback(
    // @ts-expect-error
    debounce(onSwapParamsChange, 1000),
    [onSwapParamsChange]
  )

  const flipSwapAssets = () => {
    setFromAsset(toAsset)
    setToAsset(fromAsset)

    onSwapParamsChange(
      { toOrFrom: 'from', fromAsset: toAsset, toAsset: fromAsset },
      { fromAmount, toAmount }
    )
  }

  const onSetFromAmount = (value: string) => {
    setFromAmount(value)

    onSwapParamsChangeDebounced(
      { toOrFrom: 'from', amount: value },
      { fromAmount, toAmount }
    )
  }

  const onSetToAmount = (value: string) => {
    setToAmount(value)
    onSwapParamsChangeDebounced(
      { toOrFrom: 'to', amount: value },
      { fromAmount, toAmount }
    )
  }

  const onSetExchangeRate = (value: string) => {
    setExchangeRate(value)
  }

  const onSelectExpiration = (expiration: ExpirationPresetObjectType) => {
    setOrderExpiration(expiration)
  }

  const onCustomSlippageToleranceChange = (value: string) => {
    setCustomSlippageTolerance(value)
    const customSlippage = {
      id: 4,
      slippage: Number(value)
    }
    const slippage = value ? customSlippage : slippageTolerance
    onSwapParamsChange(
      { toOrFrom: 'from', slippageTolerance: slippage },
      { fromAmount, toAmount }
    )
  }

  const onSelectSlippageTolerance = (slippage: SlippagePresetObjectType) => {
    setSlippageTolerance(slippage)
    setCustomSlippageTolerance('')
    onSwapParamsChange(
      { toOrFrom: 'from', slippageTolerance: slippage },
      { fromAmount, toAmount }
    )
  }

  const onToggleOrderType = () => {
    if (orderType === 'market') {
      setOrderType('limit')
    } else {
      setOrderType('market')
    }
  }

  const onSubmitSwap = () => {
    if (!quote) {
      return
    }

    if (!fromAsset) {
      return
    }

    if (swapValidationError === 'insufficientAllowance' && allowance) {
      // IMPORTANT SECURITY NOTICE
      //
      // The token allowance suggested by Swap is always unlimited,
      // i.e., max(uint256). While unlimited approvals are not safe from a
      // security standpoint, and this puts the entire token balance at risk
      // if 0x contracts are ever exploited, we still opted for this to give
      // users a frictionless UX and save on gas fees.
      //
      // The transaction confirmation screen for ERC20 approve() shows a loud
      // security notice, and still allows users to edit the default approval
      // amount.
      const allowanceHex = new Amount(MAX_UINT256)
        .toHex()

      approveERC20Allowance({
        from: selectedAccount.address,
        contractAddress: fromAsset.contractAddress,
        spenderAddress: quote.allowanceTarget,
        allowance: allowanceHex
      })

      return
    }

    onSwapParamsChange(
      { toOrFrom: 'from' },
      { fromAmount, toAmount },
      true
    )
  }

  const isSwapButtonDisabled = React.useMemo(() => {
    return (
      isLoading ||
      quote === undefined ||
      fromAsset === undefined ||
      toAsset === undefined ||
      (swapValidationError && swapValidationError !== 'insufficientAllowance') ||
      new Amount(toAmount).isUndefined() ||
      new Amount(toAmount).isZero() ||
      new Amount(fromAmount).isUndefined() ||
      new Amount(fromAmount).isZero()
    )
  }, [
    toAmount,
    fromAmount,
    quote,
    swapValidationError,
    isLoading,
    fromAsset,
    toAsset
  ])

  const onSelectTransactAsset = (asset: BraveWallet.BlockchainToken, toOrFrom: ToOrFromType) => {
    if (toOrFrom === 'from') {
      setFromAsset(asset)
    } else {
      setToAsset(asset)
      setToAmount('0')
    }

    setIsLoading(true)
    onSwapParamsChange(
      {
        toOrFrom,
        fromAsset: toOrFrom === 'from' ? asset : undefined,
        toAsset: toOrFrom === 'to' ? asset : undefined
      },
      { fromAmount, toAmount: '0' }
    )
    setIsLoading(false)
  }

  const onSwapInputChange = (value: string, name: string) => {
    if (name === 'to') {
      onSetToAmount(value)
    }
    if (name === 'from') {
      onSetFromAmount(value)
    }
    if (name === 'rate') {
      onSetExchangeRate(value)
    }
  }

  const onFilterAssetList = (asset: BraveWallet.BlockchainToken) => {
    const newList = swapAssetOptions.filter((assets) => assets !== asset)
    setFilteredAssetList(newList)
  }

  return {
    exchangeRate,
    filteredAssetList,
    fromAmount,
    fromAsset,
    isFetchingSwapQuote: isLoading,
    isSwapButtonDisabled,
    orderExpiration,
    orderType,
    slippageTolerance,
    swapToOrFrom,
    swapValidationError,
    toAmount,
    toAsset,
    customSlippageTolerance,
    isSwapSupported: isSupported,
    onCustomSlippageToleranceChange,
    setFromAsset,
    setSwapToOrFrom,
    onToggleOrderType,
    onSwapQuoteRefresh,
    onSetFromAmount,
    onSetToAmount,
    flipSwapAssets,
    onSubmitSwap,
    onSetExchangeRate,
    onSelectExpiration,
    onSelectSlippageTolerance,
    onSelectTransactAsset,
    onSwapInputChange,
    onFilterAssetList
  }
}
