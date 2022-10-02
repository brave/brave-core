// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import BigNumber from 'bignumber.js'

// Constants
import {
  AmountPresetTypes,
  BraveWallet,
  ExpirationPresetObjectType,
  JupiterErrorResponse,
  OrderTypes,
  SendTransactionParams,
  SlippagePresetObjectType,
  SwapErrorResponse,
  SwapValidationErrorType,
  ToOrFromType,
  WalletState
} from '../../constants/types'
import { SwapParamsPayloadType } from '../constants/action_types'
import {
  MAX_UINT256,
  NATIVE_ASSET_CONTRACT_ADDRESS_0X,
  WRAPPED_SOL_CONTRACT_ADDRESS
} from '../constants/magics'

// Options
import { SlippagePresetOptions } from '../../options/slippage-preset-options'
import { ExpirationPresetOptions } from '../../options/expiration-preset-options'
import { makeNetworkAsset } from '../../options/asset-options'

// Utils
import Amount from '../../utils/amount'
import { debounce } from '../../../common/debounce'
import { WalletActions } from '../actions'
import getAPIProxy from '../async/bridge'
import { hexStrToNumberArray } from '../../utils/hex-utils'

// Hooks
import useAssetManagement from './assets-management'
import useBalance from './balance'
import usePreset from './select-preset'
import { useIsMounted } from './useIsMounted'
import { useLib } from './useLib'

const SWAP_VALIDATION_ERROR_CODE = 100
interface Args {
  fromAsset?: BraveWallet.BlockchainToken
  toAsset?: BraveWallet.BlockchainToken
}

interface OnSwapParamsChangeOverrides {
  toOrFrom: ToOrFromType
  fromAsset?: BraveWallet.BlockchainToken
  toAsset?: BraveWallet.BlockchainToken
  amount?: string
  slippageTolerance?: SlippagePresetObjectType
}

interface OnSwapParamsChangeState {
  fromAmount: string
  toAmount: string
}

interface OnSwapParamChangeArgs {
  overrides: OnSwapParamsChangeOverrides
  state: OnSwapParamsChangeState
  full?: boolean
}

const hasDecimalsOverflow = (amount: string, asset?: BraveWallet.BlockchainToken) => {
  if (!asset) {
    return false
  }

  const amountBaseWrapped = new Amount(amount).multiplyByDecimals(asset.decimals)
  return amountBaseWrapped.value && amountBaseWrapped.value.decimalPlaces() > 0
}

export enum SwapProvider {
  ZeroEx,
  Jupiter
}

function getSwapProvider (network: BraveWallet.NetworkInfo): SwapProvider {
  switch (network.coin) {
    case BraveWallet.CoinType.ETH: return SwapProvider.ZeroEx
    case BraveWallet.CoinType.SOL: return SwapProvider.Jupiter
    default: return SwapProvider.ZeroEx
  }
}

function makeSendOptions (maxRetries?: number, preflightCommitment?: string, skipPreflight?: boolean): BraveWallet.SolanaSendTransactionOptions {
  return {
    maxRetries: maxRetries !== undefined
      ? { maxRetries: maxRetries } as unknown as BraveWallet.OptionalMaxRetries
      : undefined,
    preflightCommitment,
    skipPreflight: skipPreflight !== undefined
      ? { skipPreflight: skipPreflight }
      : undefined
  }
}

export default function useSwap ({ fromAsset: fromAssetProp, toAsset: toAssetProp }: Args = {}) {
  // redux
  const {
    selectedAccount,
    selectedNetwork,
    networkList,
    fullTokenList,
    userVisibleTokensInfo
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)
  const dispatch = useDispatch()

  const swapProvider = React.useMemo(() =>
    getSwapProvider(selectedNetwork), [selectedNetwork])

  // common state
  const [customSlippageTolerance, setCustomSlippageTolerance] = React.useState<string>('')
  const [exchangeRate, setExchangeRate] = React.useState('')
  const [filteredAssetList, setFilteredAssetList] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [fromAmount, setFromAmount] = React.useState('')
  const [fromAsset, setFromAsset] = React.useState<BraveWallet.BlockchainToken | undefined>(fromAssetProp)
  const [isLoading, setIsLoading] = React.useState<boolean>(false)
  const [isSupported, setIsSupported] = React.useState<boolean>(false)
  const [orderExpiration, setOrderExpiration] = React.useState<ExpirationPresetObjectType>(ExpirationPresetOptions[0])
  const [orderType, setOrderType] = React.useState<OrderTypes>('market')
  const [selectedPreset, setSelectedPreset] = React.useState<AmountPresetTypes | undefined>(undefined)
  const [slippageTolerance, setSlippageTolerance] = React.useState<SlippagePresetObjectType>(
    swapProvider === SwapProvider.Jupiter
      ? SlippagePresetOptions[1] // 1%
      : SlippagePresetOptions[0] // 0.5%
  )
  const [swapToOrFrom, setSwapToOrFrom] = React.useState<ToOrFromType>('from')
  const [toAmount, setToAmount] = React.useState('')
  const [toAsset, setToAsset] = React.useState<BraveWallet.BlockchainToken | undefined>(toAssetProp)

  // evm swaps (0x) state
  const [allowance, setAllowance] = React.useState<string | undefined>(undefined)
  const [swapError, setSwapError] = React.useState<SwapErrorResponse | undefined>(undefined)
  const [swapQuote, setSwapQuote] = React.useState<BraveWallet.SwapResponse | undefined>(undefined)

  // solana swaps (jupiter) state
  const [jupiterQuote, setJupiterQuote] = React.useState<BraveWallet.JupiterQuote | undefined>(undefined)
  const [jupiterError, setJupiterError] = React.useState<JupiterErrorResponse | undefined>(undefined)

  // custom hooks
  const isMounted = useIsMounted()
  const getBalance = useBalance(networkList)
  const { makeTokenVisible } = useAssetManagement()
  const {
    getIsSwapSupported,
    getERC20Allowance,
    sendSolanaSerializedTransaction,
    hasEIP1559Support
  } = useLib()

  // memos
  const nativeAsset = React.useMemo(() => makeNetworkAsset(selectedNetwork), [selectedNetwork])
  const fromAssetBalance = getBalance(selectedAccount, fromAsset)
  const nativeAssetBalance = getBalance(selectedAccount, nativeAsset)
  const toAssetBalance = getBalance(selectedAccount, toAsset)

  const feesWrapped = React.useMemo(() => {
    if (swapProvider === SwapProvider.ZeroEx) {
      if (!swapQuote) {
        return new Amount('0')
      }

      // NOTE: Swap will eventually use EIP-1559 gas fields, but we rely on
      // gasPrice as a fee-ceiling for validation of inputs.
      const { gasPrice, gas } = swapQuote
      const gasPriceWrapped = new Amount(gasPrice)
      const gasWrapped = new Amount(gas)
      return gasPriceWrapped.times(gasWrapped)
    } else if (swapProvider === SwapProvider.Jupiter) {
      // TODO: solana
    }

    return Amount.zero()
  }, [swapQuote, swapProvider])

  const swapAssetOptions: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    return [
      nativeAsset,
      ...fullTokenList.filter((asset) => asset.symbol.toUpperCase() === 'USDC'),
      ...userVisibleTokensInfo
        .filter(asset => !['USDC', nativeAsset.symbol.toUpperCase()].includes(asset.symbol.toUpperCase())),
      ...fullTokenList
        .filter(asset => !['USDC', nativeAsset.symbol.toUpperCase()].includes(asset.symbol.toUpperCase()))
        .filter(asset => !userVisibleTokensInfo
          .some(token => token.symbol.toUpperCase() === asset.symbol.toUpperCase()))
    ].filter(asset => (
      asset.chainId === selectedNetwork.chainId &&
      !asset.isErc721 // NFT swaps not supported
    ))
  }, [fullTokenList, userVisibleTokensInfo, nativeAsset, selectedNetwork])

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

    // No balance-based validations to perform when FROM/native balances
    // have not been fetched yet.
    if (!fromAssetBalance || !nativeAssetBalance) {
      return
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

    // 0x specific validations
    if (swapProvider === SwapProvider.ZeroEx) {
      if (allowance !== undefined &&
          new Amount(allowance).lt(fromAmountWeiWrapped)) {
        return 'insufficientAllowance'
      }

      if (swapError === undefined) {
        return
      }

      const { code, validationErrors } = swapError
      switch (code) {
        case SWAP_VALIDATION_ERROR_CODE:
          if (validationErrors?.find(err => err.reason === 'INSUFFICIENT_ASSET_LIQUIDITY')) {
            return 'insufficientLiquidity'
          }
          break

        default:
          return 'unknownError'
      }
    }

    // Jupiter specific validations
    if (swapProvider === SwapProvider.Jupiter) {
      if (jupiterError === undefined) {
        return
      }

      if (jupiterError.message.includes('No routes found for the input and output mints')) {
        return 'insufficientLiquidity'
      }

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
    swapError,
    allowance,
    swapProvider,
    jupiterError
  ])

  const isSwapButtonDisabled = React.useMemo(() => {
    return (
      // Prevent creating a swap transaction with stale parameters if fetching
      // of a new quote is in progress.
      isLoading ||

      // If 0x swap quote is empty, there's nothing to create the swap
      // transaction with, so Swap button must be disabled.
      (swapProvider === SwapProvider.ZeroEx && swapQuote === undefined) ||

      // If Jupiter quote is empty, there's nothing to create the swap
      // transaction with, so Swap button must be disabled.
      (swapProvider === SwapProvider.Jupiter && jupiterQuote === undefined) ||

      // FROM/TO assets may be undefined during initialization of the swap
      // assets list.
      fromAsset === undefined ||
      toAsset === undefined ||

      // Amounts must be defined by the user, or populated from the swap quote,
      // for creating a transaction.
      new Amount(toAmount).isUndefined() ||
      new Amount(toAmount).isZero() ||
      new Amount(fromAmount).isUndefined() ||
      new Amount(fromAmount).isZero() ||

      // Disable Swap button if native asset balance has not been fetched yet,
      // to ensure insufficientFundsForGas error (if applicable) is accurate.
      new Amount(nativeAssetBalance).isUndefined() ||

      // Disable Swap button if FROM asset balance has not been fetched yet,
      // to ensure insufficientBalance error (if applicable) is accurate.
      new Amount(fromAssetBalance).isUndefined() ||

      // Unless the validation error is insufficientAllowance, in which case
      // the transaction is an ERC20Approve, Swap button must be disabled.
      (swapValidationError && (swapProvider === SwapProvider.ZeroEx &&
          swapValidationError !== 'insufficientAllowance'))
    )
  }, [
    isLoading,
    fromAsset,
    toAsset,
    fromAmount,
    toAmount,
    nativeAssetBalance,
    fromAssetBalance,
    swapValidationError,
    swapProvider,
    swapQuote,
    jupiterQuote
  ])

  // Callbacks / methods
  const hasToken = React.useCallback((token: BraveWallet.BlockchainToken) =>
    swapAssetOptions.some(option =>
      option.chainId === token.chainId &&
      option.contractAddress.toLowerCase() === token.contractAddress.toLowerCase()
    ),
    [swapAssetOptions]
  )

  const setToAssetAndMakeVisible = React.useCallback((token?: BraveWallet.BlockchainToken) => {
    setToAsset(token)

    if (token) {
      makeTokenVisible(token)
    }
  }, [makeTokenVisible])

  const fetch0xSwapQuote = React.useCallback(async (payload: SwapParamsPayloadType) => {
    if (!isMounted) {
      return
    }

    const { swapService, ethTxManagerProxy } = getAPIProxy()

    const {
      fromAsset,
      fromAssetAmount,
      toAsset,
      toAssetAmount,
      accountAddress,
      slippageTolerance,
      full
    } = payload

    const swapParams: BraveWallet.SwapParams = {
      takerAddress: accountAddress,
      sellAmount: fromAssetAmount || '',
      buyAmount: toAssetAmount || '',
      buyToken: toAsset.contractAddress || NATIVE_ASSET_CONTRACT_ADDRESS_0X,
      sellToken: fromAsset.contractAddress || NATIVE_ASSET_CONTRACT_ADDRESS_0X,
      slippagePercentage: slippageTolerance.slippage / 100,
      gasPrice: ''
    } as BraveWallet.SwapParams

    const quote = await (
      full ? swapService.getTransactionPayload(swapParams) : swapService.getPriceQuote(swapParams)
    )

    if (quote.success && quote.response) {
      if (isMounted) {
        setSwapError(undefined)
        setSwapQuote(quote.response)
      }

      if (full) {
        const {
          to,
          data,
          value,
          estimatedGas
        } = quote.response

        // Get the latest gas estimates, since we'll force the fastest fees in
        // order to ensure a swap with minimum slippage.
        const { estimation: gasEstimates } = await ethTxManagerProxy.getGasEstimation1559()

        const isEIP1559 = hasEIP1559Support(selectedAccount, selectedNetwork) && gasEstimates
        let maxPriorityFeePerGas
        let maxFeePerGas
        if (isEIP1559 && gasEstimates.fastMaxPriorityFeePerGas === gasEstimates.avgMaxPriorityFeePerGas) {
          // Bump fast priority fee and max fee by 1 GWei if same as average fees.
          const maxPriorityFeePerGasBN = new BigNumber(gasEstimates.fastMaxPriorityFeePerGas).plus(10 ** 9)
          const maxFeePerGasBN = new BigNumber(gasEstimates.fastMaxFeePerGas).plus(10 ** 9)

          maxPriorityFeePerGas = `0x${maxPriorityFeePerGasBN.toString(16)}`
          maxFeePerGas = `0x${maxFeePerGasBN.toString(16)}`
        } else if (isEIP1559) {
          // Always suggest fast gas fees as default
          maxPriorityFeePerGas = gasEstimates.fastMaxPriorityFeePerGas
          maxFeePerGas = gasEstimates.fastMaxFeePerGas
        }

        const params: SendTransactionParams = {
          from: accountAddress,
          to,
          value: new Amount(value).toHex(),
          gas: new Amount(estimatedGas).toHex(),
          data: hexStrToNumberArray(data),
          maxPriorityFeePerGas,
          maxFeePerGas,
          coin: BraveWallet.CoinType.ETH
        }

        dispatch(WalletActions.sendTransaction(params))

        if (isMounted) {
          setSwapError(undefined)
          setSwapQuote(undefined)
        }
      }
    } else if (quote.errorResponse) {
      try {
        const err = JSON.parse(quote.errorResponse) as SwapErrorResponse
        if (isMounted) {
          setSwapError(err)
        }
      } catch (e) {
        console.error(`[swap] error parsing response: ${e}`)
      } finally {
        console.error(`[swap] error querying 0x API: ${quote.errorResponse}`)
      }
    }
  }, [isMounted, selectedAccount, selectedNetwork])

  const fetchJupiterSwapQuote = React.useCallback(async (payload: SwapParamsPayloadType) => {
    if (!isMounted) {
      return
    }

    const { swapService } = getAPIProxy()

    const {
      fromAsset,
      fromAssetAmount,
      toAsset,
      accountAddress,
      slippageTolerance,
      full
    } = payload

    if (!full) {
      const quote = await swapService.getJupiterQuote({
        inputMint: fromAsset.contractAddress || WRAPPED_SOL_CONTRACT_ADDRESS,
        outputMint: toAsset.contractAddress || WRAPPED_SOL_CONTRACT_ADDRESS,
        amount: fromAssetAmount || '',
        slippagePercentage: slippageTolerance.slippage
      })

      if (quote.success && quote.response) {
        if (isMounted) {
          setJupiterQuote(quote.response)
          setJupiterError(undefined)
        }
      } else if (quote.errorResponse) {
        try {
          const err = JSON.parse(quote.errorResponse) as JupiterErrorResponse
          if (isMounted) {
            setJupiterError(err)
          }
        } catch (e) {
          console.error(`[swap] error parsing Jupiter response: ${e}`)
        } finally {
          console.error(`[swap] error querying Jupiter API: ${quote.errorResponse}`)
        }
      }
    } else {
      if (!jupiterQuote || jupiterQuote?.routes.length === 0) {
        return
      }

      const swapTransactions = await swapService.getJupiterSwapTransactions({
        userPublicKey: accountAddress,
        route: jupiterQuote.routes[0],
        outputMint: toAsset.contractAddress || WRAPPED_SOL_CONTRACT_ADDRESS
      })

      if (swapTransactions.success && swapTransactions.response) {
        const {
          setupTransaction,
          swapTransaction,
          cleanupTransaction
        } = swapTransactions.response
        const serializedTransactions = [
          setupTransaction,
          swapTransaction,
          cleanupTransaction
        ].filter(txn => txn !== '')

        const hasParts = serializedTransactions.length > 1
        const groupId = hasParts
          // @ts-expect-error
          ? crypto.randomUUID()
          : undefined

        await Promise.all(serializedTransactions.map(async (each, idx) => {
          await sendSolanaSerializedTransaction({
            encodedTransaction: each,
            from: accountAddress,
            txType: !hasParts || idx === 1
              ? BraveWallet.TransactionType.SolanaSwap
              : BraveWallet.TransactionType.Other,
            sendOptions: makeSendOptions(undefined, undefined, true),
            groupId
          })
        }))

        if (isMounted) {
          setJupiterQuote(undefined)
          setJupiterError(undefined)
        }
      } else if (swapTransactions.errorResponse) {
        try {
          const err = JSON.parse(swapTransactions.errorResponse) as JupiterErrorResponse
          if (isMounted) {
            setJupiterError(err)
          }
        } catch (e) {
          console.error(`[swap] error parsing Jupiter response: ${e}`)
        } finally {
          console.error(`[swap] error querying Jupiter swap API: ${swapTransactions.errorResponse}`)
        }
      }
    }
  }, [isMounted, jupiterQuote])

  /**
   * onSwapParamsChange() is triggered whenever a change in the swap fields
   * requires fetching the swap quote.
   *
   * @param {Object} overrides Override fields from the React state.
   * @param {Object} state     State fields used to prevent recreating the
   *                           function on change.
   */
  const onSwapParamsChange = React.useCallback(async (
    {
      full = false,
      overrides,
      state
    }: OnSwapParamChangeArgs
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
    const params = {
      fromAsset: fromAssetNext,
      fromAssetAmount: fromAmountWeiWrapped?.format(),
      toAsset: toAssetNext,
      toAssetAmount: toAmountWeiWrapped?.format(),
      accountAddress: selectedAccount.address,
      slippageTolerance: overrides.slippageTolerance ?? slippageTolerance,
      networkChainId: selectedNetwork.chainId,
      full
    } as SwapParamsPayloadType

    if (swapProvider === SwapProvider.ZeroEx) {
      await fetch0xSwapQuote(params)
    } else if (swapProvider === SwapProvider.Jupiter) {
      await fetchJupiterSwapQuote(params)
    }
  }, [
    selectedAccount,
    selectedNetwork,
    fromAsset,
    toAsset,
    swapProvider,
    fetch0xSwapQuote,
    fetchJupiterSwapQuote
  ])

  const onSwapQuoteRefresh = React.useCallback(async () => {
    const customSlippage = {
      id: 4,
      slippage: Number(customSlippageTolerance)
    }
    await onSwapParamsChange({
      overrides: { toOrFrom: 'from', slippageTolerance: customSlippageTolerance ? customSlippage : slippageTolerance },
      state: { fromAmount, toAmount }
    })
  }, [onSwapParamsChange, fromAmount, toAmount, customSlippageTolerance, slippageTolerance])

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
    (debounce<OnSwapParamChangeArgs>(onSwapParamsChange, 1000)),
    [onSwapParamsChange]
  )

  const flipSwapAssets = React.useCallback(() => {
    setFromAsset(toAsset)
    setToAssetAndMakeVisible(fromAsset)

    clearPreset()

    onSwapParamsChange({
      overrides: { toOrFrom: 'from', fromAsset: toAsset, toAsset: fromAsset },
      state: { fromAmount, toAmount }
    })
  }, [toAsset, fromAsset, fromAmount, toAmount, onSwapParamsChange, setToAssetAndMakeVisible])

  const onSetFromAmount = React.useCallback(async (value: string) => {
    setFromAmount(value)

    await onSwapParamsChangeDebounced({
      overrides: { toOrFrom: 'from', amount: value },
      state: { fromAmount, toAmount }
    })
  }, [onSwapParamsChangeDebounced, fromAmount, toAmount])

  const onSetToAmount = React.useCallback(async (value: string) => {
    setToAmount(value)
    await onSwapParamsChangeDebounced({
      overrides: { toOrFrom: 'to', amount: value },
      state: { fromAmount, toAmount }
    })
  }, [onSwapParamsChangeDebounced, fromAmount, toAmount])

  const onCustomSlippageToleranceChange = React.useCallback((value: string) => {
    setCustomSlippageTolerance(value)
    const customSlippage = {
      id: 4,
      slippage: Number(value)
    }
    const slippage = value ? customSlippage : slippageTolerance
    onSwapParamsChange({
      overrides: { toOrFrom: 'from', slippageTolerance: slippage },
      state: { fromAmount, toAmount }
    })
  }, [onSwapParamsChange, fromAmount, toAmount, slippageTolerance])

  const onSelectSlippageTolerance = React.useCallback((slippage: SlippagePresetObjectType) => {
    setSlippageTolerance(slippage)
    setCustomSlippageTolerance('')
    onSwapParamsChange({
      overrides: { toOrFrom: 'from', slippageTolerance: slippage },
      state: { fromAmount, toAmount }
    })
  }, [onSwapParamsChange, fromAmount, toAmount])

  const onToggleOrderType = React.useCallback(() => {
    if (orderType === 'market') {
      setOrderType('limit')
    } else {
      setOrderType('market')
    }
  }, [orderType])

  const onSubmitSwap = React.useCallback(() => {
    if (!fromAsset) {
      return
    }

    if (swapProvider === SwapProvider.ZeroEx) {
      if (!swapQuote) {
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

        dispatch(WalletActions.approveERC20Allowance({
          from: selectedAccount.address,
          contractAddress: fromAsset.contractAddress,
          spenderAddress: swapQuote.allowanceTarget,
          allowance: allowanceHex
        }))
        return
      }
    } else if (swapProvider === SwapProvider.Jupiter) {
      if (!jupiterQuote) {
        return
      }
    }

    onSwapParamsChange({
      overrides: { toOrFrom: 'from' },
      state: { fromAmount, toAmount },
      full: true
    })
  }, [
    fromAsset,
    swapValidationError,
    allowance,
    selectedAccount,
    fromAmount,
    toAmount,
    swapProvider,
    swapQuote,
    jupiterQuote
  ])

  const onSelectTransactAsset = React.useCallback((asset: BraveWallet.BlockchainToken, toOrFrom: ToOrFromType) => {
    if (toOrFrom === 'from') {
      setFromAsset(asset)
    } else {
      setToAssetAndMakeVisible(asset)
      setToAmount('0')
    }

    setIsLoading(true)
    onSwapParamsChange({
      overrides: {
        toOrFrom,
        fromAsset: toOrFrom === 'from' ? asset : undefined,
        toAsset: toOrFrom === 'to' ? asset : undefined
      },
      state: { fromAmount, toAmount: '0' }
    })
    setIsLoading(false)
  }, [onSwapParamsChange, setToAssetAndMakeVisible, fromAmount])

  const onSwapInputChange = React.useCallback((value: string, name: 'to' | 'from' | 'rate') => {
    if (name === 'to') {
      onSetToAmount(value)
    }
    if (name === 'from') {
      onSetFromAmount(value)
    }
    if (name === 'rate') {
      setExchangeRate(value)
    }
  }, [onSetToAmount, onSetFromAmount, setExchangeRate])

  const onFilterAssetList = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const newList = swapAssetOptions.filter((assets) => assets !== asset)
    setFilteredAssetList(newList)
  }, [swapAssetOptions])

  const clearPreset = React.useCallback(() => setSelectedPreset(undefined), [])

  const onSelectPresetAmount = usePreset(
    {
      onSetAmount: onSetFromAmount,
      asset: fromAsset
    }
  )

  // Effects
  React.useEffect(() => {
    // This hook is triggered whenever swapAssetOptions changes, updating
    // the from/to assets, typically when userVisibleTokensInfo is updated
    // in the following three ways:
    //   1. Redux initialisation.
    //   2. User updates Visible Assets.
    //   3. User selects a taker asset that is not part of
    //      userVisibleTokensInfo.

    setFilteredAssetList(swapAssetOptions)

    if (!fromAsset || !hasToken(fromAsset)) {
      setFromAsset(swapAssetOptions[0])
    }

    if (!toAsset || !hasToken(toAsset)) {
      setToAsset(swapAssetOptions[1])
    }
  }, [swapAssetOptions, fromAsset, toAsset])

  React.useEffect(() => {
    let isSubscribed = true // track if the component is mounted

    getIsSwapSupported(selectedNetwork).then(
      (supported) => {
        if (isSubscribed) {
          try {
            setIsSupported(supported)
          } catch (error) {
            console.log(error)
          }
        }
      }
    ).catch(error => console.log(error))

    // cleanup function, unsubscribe to promise on unmount
    return () => {
      isSubscribed = false
    }
  }, [selectedNetwork])

  React.useEffect(() => {
    let isSubscribed = true // track if the component is mounted

    if (swapProvider !== SwapProvider.ZeroEx) {
      return
    }

    if (!fromAsset) {
      return
    }

    if (!fromAsset.isErc20) {
      setAllowance(undefined)
      return
    }

    if (!swapQuote) {
      setAllowance(undefined)
      return
    }

    const { allowanceTarget } = swapQuote

    getERC20Allowance(fromAsset.contractAddress, selectedAccount.address, allowanceTarget)
      .then(value => {
        if (isSubscribed) {
          setAllowance(value)
        }
      })
      .catch(e => console.log(e))

    // cleanup function, unsubscribe to promise on unmount
    return () => {
      isSubscribed = false
    }
  }, [fromAsset, swapQuote, selectedAccount, swapProvider])

  /**
   * React effect to extract fields from the 0x swap quote and write the relevant
   * fields to the state.
   */
  React.useEffect(() => {
    if (swapProvider !== SwapProvider.ZeroEx) {
      return
    }

    if (!swapQuote) {
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
    } = swapQuote

    const newFromAmount = new Amount(sellAmount)
      .divideByDecimals(fromAsset.decimals)
      .format()

    setFromAmount(newFromAmount)

    const newToAmount = new Amount(buyAmount)
      .divideByDecimals(toAsset.decimals)
      .format()

    setToAmount(newToAmount)

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
  }, [swapQuote, swapProvider])

  /**
   * React effect to extract fields from the Jupiter swap quote and write the
   * relevant fields to the state.
   */
  React.useEffect(() => {
    if (swapProvider !== SwapProvider.Jupiter) {
      return
    }

    if (!jupiterQuote) {
      setFromAmount('')
      setToAmount('')
      setExchangeRate('')
      return
    }

    if (!fromAsset || !toAsset) {
      return
    }

    const { routes } = jupiterQuote
    if (routes.length === 0) {
      return
    }

    const [{ inAmount, otherAmountThreshold }] = routes

    const newFromAmountWrapped = new Amount(inAmount.toString())
      .divideByDecimals(fromAsset.decimals)
    setFromAmount(newFromAmountWrapped.format())

    const newToAmountWrapped = new Amount(otherAmountThreshold.toString())
      .divideByDecimals(toAsset.decimals)
    setToAmount(newToAmountWrapped.format())

    /**
     * Price computation block
     *
     * No exchange rate is returned by Jupiter API, so we estimate it from the
     * quote.
     */
    const priceWrapped = newToAmountWrapped.div(newFromAmountWrapped)
    setExchangeRate(priceWrapped.format(6))
  }, [jupiterQuote, swapProvider])

  // Set isLoading to false as soon as:
  //  - swap quote has been fetched.
  //  - error from 0x API is available in rawError.
  React.useEffect(() => setIsLoading(false), [swapQuote, jupiterQuote, swapError, jupiterError])

  return {
    customSlippageTolerance,
    exchangeRate,
    filteredAssetList,
    flipSwapAssets,
    fromAmount,
    fromAsset,
    fromAssetBalance,
    isFetchingSwapQuote: isLoading,
    isSwapButtonDisabled,
    isSwapSupported: isSupported,
    onCustomSlippageToleranceChange,
    onFilterAssetList,
    setOrderExpiration,
    onSelectPresetAmount,
    onSelectSlippageTolerance,
    onSelectTransactAsset,
    onSubmitSwap,
    onSwapInputChange,
    onSwapQuoteRefresh,
    onToggleOrderType,
    orderExpiration,
    orderType,
    selectedPreset,
    setSelectedPreset,
    setSwapToOrFrom,
    slippageTolerance,
    swapAssetOptions,
    swapToOrFrom,
    swapValidationError,
    toAmount,
    toAsset,
    toAssetBalance,
    swapProvider,

    // For use in unit-tests
    setSwapError,
    setSwapQuote,
    allowance,
    setAllowance
  }
}
