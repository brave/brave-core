// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SimpleActionCreator } from 'redux-act'
import BigNumber from 'bignumber.js'

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
import { SlippagePresetOptions } from '../../options/slippage-preset-options'
import { ExpirationPresetOptions } from '../../options/expiration-preset-options'
import { formatInputValue, toHex, toWei, toWeiHex } from '../../utils/format-balances'
import { debounce } from '../../../common/debounce'
import { SwapParamsPayloadType } from '../constants/action_types'
import useBalance from './balance'

const SWAP_VALIDATION_ERROR_CODE = 100

export default function useSwap (
  selectedAccount: WalletAccountType,
  selectedNetwork: BraveWallet.EthereumChain,
  swapAssetOptions: BraveWallet.BlockchainToken[],
  fetchSwapQuote: SimpleActionCreator<SwapParamsPayloadType>,
  getERC20Allowance: (contractAddress: string, ownerAddress: string, spenderAddress: string) => Promise<string>,
  approveERC20Allowance: SimpleActionCreator<ApproveERC20Params>,
  isSwapSupported: (network: BraveWallet.EthereumChain) => Promise<boolean>,
  quote?: BraveWallet.SwapResponse,
  rawError?: SwapErrorResponse
) {
  const [exchangeRate, setExchangeRate] = React.useState('')
  const [fromAmount, setFromAmount] = React.useState('')
  const [fromAsset, setFromAsset] = React.useState<BraveWallet.BlockchainToken>(swapAssetOptions[0])
  const [orderExpiration, setOrderExpiration] = React.useState<ExpirationPresetObjectType>(ExpirationPresetOptions[0])
  const [orderType, setOrderType] = React.useState<OrderTypes>('market')
  const [slippageTolerance, setSlippageTolerance] = React.useState<SlippagePresetObjectType>(SlippagePresetOptions[0])
  const [customSlippageTolerance, setCustomSlippageTolerance] = React.useState<string>('')
  const [toAmount, setToAmount] = React.useState('')
  const [toAsset, setToAsset] = React.useState<BraveWallet.BlockchainToken>(swapAssetOptions[1])
  const [filteredAssetList, setFilteredAssetList] = React.useState<BraveWallet.BlockchainToken[]>(swapAssetOptions)
  const [swapToOrFrom, setSwapToOrFrom] = React.useState<ToOrFromType>('from')
  const [allowance, setAllowance] = React.useState<string | undefined>(undefined)
  const [isLoading, setIsLoading] = React.useState<boolean>(false)
  const [isSupported, setIsSupported] = React.useState<boolean>(false)

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
  const ethBalance = getBalance(selectedAccount)

  const feesBN = React.useMemo(() => {
    if (!quote) {
      return new BigNumber('0')
    }

    // NOTE: Swap will eventually use EIP-1559 gas fields, but we rely on
    // gasPrice as a fee-ceiling for validation of inputs.
    const { gasPrice, gas } = quote
    const gasPriceBN = new BigNumber(gasPrice)
    const gasBN = new BigNumber(gas)
    return gasPriceBN.multipliedBy(gasBN)
  }, [quote])

  const hasDecimalsOverflow = React.useCallback(
    (amount: string, asset?: BraveWallet.BlockchainToken) => {
      if (!asset) {
        return false
      }

      return new BigNumber(amount).times(10 ** asset.decimals).decimalPlaces() > 0
    }, []
  )

  const swapValidationError: SwapValidationErrorType | undefined = React.useMemo(() => {
    if (hasDecimalsOverflow(fromAmount, fromAsset)) {
      return 'fromAmountDecimalsOverflow'
    }

    if (hasDecimalsOverflow(toAmount, toAsset)) {
      return 'toAmountDecimalsOverflow'
    }

    const fromAmountWei = toWei(fromAmount, fromAsset.decimals)
    const amountBN = new BigNumber(fromAmountWei)
    const balanceBN = new BigNumber(fromAssetBalance)
    const ethBalanceBN = new BigNumber(ethBalance)

    if (amountBN.gt(balanceBN)) {
      return 'insufficientBalance'
    }

    if (feesBN.gt(ethBalanceBN)) {
      return 'insufficientEthBalance'
    }

    if (fromAsset.symbol === selectedNetwork.symbol && amountBN.plus(feesBN).gt(balanceBN)) {
      return 'insufficientEthBalance'
    }

    if (allowance !== undefined && new BigNumber(allowance).lt(amountBN)) {
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
    ethBalance,
    feesBN,
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
      return
    }

    const { buyAmount, sellAmount, price, buyTokenToEthRate, sellTokenToEthRate } = quote

    setFromAmount(formatInputValue(sellAmount, fromAsset.decimals))
    setToAmount(formatInputValue(buyAmount, toAsset.decimals))

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
    const priceBN = new BigNumber(price)
    const approxPriceBN = new BigNumber(buyTokenToEthRate).dividedBy(sellTokenToEthRate)
    let bestEstimatePriceBN
    if (approxPriceBN.div(priceBN).toFixed(0) === '1') {
      bestEstimatePriceBN = priceBN
    } else if (priceBN.div(approxPriceBN).toFixed(0) === '1') {
      bestEstimatePriceBN = priceBN
    } else {
      bestEstimatePriceBN = approxPriceBN
    }

    const bestEstimatePrice = bestEstimatePriceBN.toFixed(4, BigNumber.ROUND_UP)
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
    // if (selectedWidgetTab !== 'swap') {
    //   return
    // }

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

    let fromAmountWei
    let toAmountWei

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

      fromAmountWei = toWei(
        overrides.amount ?? state.fromAmount,
        fromAssetNext.decimals
      )
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
        toAmountWei = toWei(
          overrides.amount ?? state.toAmount,
          toAssetNext.decimals
        )
      } else {
        fromAmountWei = toWei(
          state.fromAmount,
          fromAssetNext.decimals
        )
      }
    }

    /**
     * STEP 4: Reset amount fields that effectively have zero value.
     *
     * The following block makes it impossible to enter 0-ish amount
     * values.
     */
    if (toAmountWei === '0') {
      setToAmount('')
      return
    }

    if (fromAmountWei === '0') {
      setFromAmount('')
      return
    }

    /**
     * STEP 5: Fetch the swap quote asynchronously.
     */
    setIsLoading(true)
    fetchSwapQuote({
      fromAsset: fromAssetNext,
      fromAssetAmount: fromAmountWei,
      toAsset: toAssetNext,
      toAssetAmount: toAmountWei,
      accountAddress: selectedAccount.address,
      slippageTolerance: overrides.slippageTolerance ?? slippageTolerance,
      networkChainId: selectedNetwork.chainId,
      full
    })
  }, [selectedAccount, selectedNetwork, fromAsset, toAsset])

  // Set isLoading to false as soon as quote has been fetched.
  React.useEffect(() => setIsLoading(false), [quote])

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

    if (swapValidationError === 'insufficientAllowance' && allowance) {
      const allowanceHex = !fromAssetBalance
        ? toWeiHex(fromAmount, fromAsset.decimals)
        : toHex(fromAssetBalance)

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

  const isSwapButtonDisabled = React.useMemo(() => (
    isLoading ||
    quote === undefined ||
    (swapValidationError && swapValidationError !== 'insufficientAllowance') ||
    toWei(toAmount, toAsset.decimals) === '0' ||
    toWei(fromAmount, fromAsset.decimals) === '0'
  ), [toAmount, fromAmount, toAsset, fromAsset, quote, swapValidationError, isLoading])

  const onSelectTransactAsset = (asset: BraveWallet.BlockchainToken, toOrFrom: ToOrFromType) => {
    if (toOrFrom === 'from') {
      setFromAsset(asset)
    } else {
      setToAsset(asset)
    }

    onSwapParamsChange(
      {
        toOrFrom,
        fromAsset: toOrFrom === 'from' ? asset : undefined,
        toAsset: toOrFrom === 'to' ? asset : undefined
      },
      { fromAmount, toAmount }
    )
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
