// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useEffect, useMemo, useState } from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useHistory } from 'react-router'

// Options
import { SwapAndSendOptions } from '../../../../options/swap-and-send-options'
import { gasFeeOptions } from '../../../../options/gas-fee-options'

// Hooks
import { useJupiter } from './useJupiter'
import { useZeroEx } from './useZeroEx'
import { useLifi } from './useLifi'
import { useDebouncedCallback } from './useDebouncedCallback'
import {
  useScopedBalanceUpdater //
} from '../../../../common/hooks/use-scoped-balance-updater'
import { useQuery } from '../../../../common/hooks/use-query'
import {
  useTokenAllowance //
} from '../../../../common/hooks/use_token_allowance'

// Types and constants
import {
  GasEstimate,
  SwapValidationErrorType,
  SwapParamsOverrides,
  SwapParams
} from '../constants/types'
import {
  BraveWallet,
  GasFeeOption,
  WalletRoutes
} from '../../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import Amount from '../../../../utils/amount'
import { getPriceIdForToken } from '../../../../utils/api-utils'
// FIXME(onyb): move makeNetworkAsset to utils/assets-utils
import { isNativeAsset } from '../../../../utils/asset-utils'
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
  getJupiterToAmount,
  getLiFiQuoteOptions,
  getLiFiFromAmount,
  getLiFiToAmount
} from '../swap.utils'
import { makeSwapRoute } from '../../../../utils/routes-utils'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetSwapSupportedNetworksQuery,
  useGetTokenSpotPricesQuery,
  useGenerateSwapQuoteMutation
} from '../../../../common/slices/api.slice'
import { querySubscriptionOptions60s } from '../../../../common/slices/constants'
import { AccountInfoEntity } from '../../../../common/slices/entities/account-info.entity'
import {
  useAccountFromAddressQuery,
  useGetCombinedTokensListQuery
} from '../../../../common/slices/api.slice.extra'

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

const getTokenFromParam = (
  contractOrSymbol: string,
  network: BraveWallet.NetworkInfo,
  tokenList: BraveWallet.BlockchainToken[]
) => {
  return tokenList.find(
    (token) =>
      (token.chainId === network.chainId &&
        token.coin === network.coin &&
        token.contractAddress.toLowerCase() ===
          contractOrSymbol.toLowerCase()) ||
      (token.chainId === network.chainId &&
        token.coin === network.coin &&
        token.contractAddress === '' &&
        token.symbol.toLowerCase() === contractOrSymbol.toLowerCase())
  )
}

export const useSwap = () => {
  // routing
  const query = useQuery()
  const history = useHistory()

  // Queries
  // FIXME(onyb): what happens when defaultFiatCurrency is empty
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: supportedNetworks } = useGetSwapSupportedNetworksQuery()
  const { data: fullTokenList } = useGetCombinedTokensListQuery()
  const { account: fromAccount } = useAccountFromAddressQuery(
    query.get('fromAccountId') ?? undefined
  )
  // TODO: deprecate toAccountId in favour of toAddress + toCoin
  const toAccountId = fromAccount?.accountId
  const toCoinFromParams = query.get('toCoin') ?? undefined
  const toCoin = toCoinFromParams ? Number(toCoinFromParams) : undefined
  const toAddress = query.get('toAddress') ?? undefined

  // State
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
  const [selectedSwapSendAccount, setSelectedSwapSendAccount] = useState<
    AccountInfoEntity | undefined
  >(undefined)

  // Mutations
  const [generateSwapQuote] = useGenerateSwapQuoteMutation()

  // Memos
  const fromNetwork = useMemo(() => {
    if (!supportedNetworks?.length) {
      return
    }
    return supportedNetworks.find(
      (network) =>
        network.chainId === query.get('fromChainId') &&
        network.coin === fromAccount?.accountId.coin
    )
  }, [supportedNetworks, fromAccount?.accountId.coin, query])

  const toNetwork = useMemo(() => {
    if (!supportedNetworks?.length || !toCoin) {
      return
    }
    return supportedNetworks.find(
      (network) =>
        network.chainId === query.get('toChainId') && network.coin === toCoin
    )
  }, [supportedNetworks, toCoin, query])

  const fromToken = useMemo(() => {
    const contractOrSymbol = query.get('fromToken')
    if (!contractOrSymbol || !fromNetwork) {
      return
    }

    return getTokenFromParam(contractOrSymbol, fromNetwork, fullTokenList)
  }, [fullTokenList, query, fromNetwork])

  const toToken = useMemo(() => {
    const contractOrSymbol = query.get('toToken')
    if (!contractOrSymbol || !toNetwork) {
      return
    }

    return getTokenFromParam(contractOrSymbol, toNetwork, fullTokenList)
  }, [fullTokenList, query, toNetwork])

  const nativeAsset = useMemo(
    () => makeNetworkAsset(fromNetwork),
    [fromNetwork]
  )

  const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
    useScopedBalanceUpdater(
      fromAccount && fromNetwork && fromToken && nativeAsset
        ? {
            network: fromNetwork,
            accounts: [fromAccount],
            tokens: isNativeAsset(fromToken)
              ? [nativeAsset]
              : [nativeAsset, fromToken]
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

    if (quoteUnion.lifiQuote) {
      return getLiFiQuoteOptions({
        quote: quoteUnion.lifiQuote,
        fromNetwork,
        spotPrices: spotPriceRegistry,
        defaultFiatCurrency,
        toToken,
        fromToken
      })
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

  const swapProviderHookParams: SwapParams = useMemo(() => {
    return {
      fromNetwork,
      fromAccount,
      fromToken,
      fromAmount: editingFromOrToAmount === 'from' ? fromAmount : '',
      toAccountId: toAccountId,
      toToken,
      toAmount: editingFromOrToAmount === 'to' ? toAmount : '',
      slippageTolerance
    }
  }, [
    fromNetwork,
    fromAccount,
    fromToken,
    editingFromOrToAmount,
    fromAmount,
    toAccountId,
    toToken,
    toAmount,
    slippageTolerance
  ])

  const jupiter = useJupiter(swapProviderHookParams)
  const zeroEx = useZeroEx(swapProviderHookParams)
  const lifi = useLifi(swapProviderHookParams)
  const { approveSpendAllowance, checkAllowance, hasAllowance } =
    useTokenAllowance()

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
      if (!fromAccount || !toAccountId || !fromNetwork) {
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
          toAccountId: toAccountId,
          toChainId: params.toToken.chainId,
          toAmount:
            params.editingFromOrToAmount === 'to' && params.toAmount
              ? new Amount(params.toAmount)
                  .multiplyByDecimals(params.toToken.decimals)
                  .format()
              : '',
          toToken: params.toToken.contractAddress,
          slippagePercentage: slippageTolerance,
          routePriority:
            params.fromToken.chainId === params.toToken.chainId
              ? BraveWallet.RoutePriority.kCheapest
              : BraveWallet.RoutePriority.kRecommended
        }).unwrap()
      } catch (e) {
        setIsFetchingQuote(false)
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

        if (quoteResponse.response.lifiQuote) {
          const { routes } = quoteResponse.response.lifiQuote

          // TODO(douglas): refactor to allow selecting routes
          const route = routes[0] || undefined

          if (!route) {
            return
          }

          if (params.editingFromOrToAmount === 'from') {
            setToAmount(getLiFiToAmount(route).format(6))
          } else {
            setFromAmount(getLiFiFromAmount(route).format(6))
          }

          const step = route.steps[0]

          if (step) {
            await checkAllowance({
              account: fromAccount,
              spendAmount: step.estimate.fromAmount,
              spenderAddress: step.estimate.approvalAddress,
              token: params.fromToken
            })
          }
        }

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

          await checkAllowance({
            account: fromAccount,
            spendAmount: quoteResponse.response.zeroExQuote.sellAmount,
            spenderAddress: quoteResponse.response.zeroExQuote.allowanceTarget,
            token: params.fromToken
          })
        }
      }

      if (quoteResponse?.fees) {
        setSwapFees(quoteResponse?.fees)
      }

      setIsFetchingQuote(false)
      setAbortController(undefined)
    },
    [
      fromAccount,
      toAccountId,
      fromNetwork,
      fromAmount,
      toAmount,
      fromToken,
      toToken,
      editingFromOrToAmount,
      reset,
      generateSwapQuote,
      slippageTolerance,
      checkAllowance
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
    if (!fromAccount || !fromToken || !toToken) {
      return
    }
    if (
      fromAccount.accountId.coin !== toToken.coin ||
      toCoin !== fromToken.coin
    ) {
      history.replace(WalletRoutes.Swap)
    } else {
      history.replace(
        makeSwapRoute({
          fromToken: toToken,
          fromAccount,
          toToken: fromToken,
          toAddress,
          toCoin
        })
      )
    }
    await handleOnSetFromAmount('')
  }, [
    fromAccount,
    fromToken,
    toToken,
    toCoin,
    handleOnSetFromAmount,
    history,
    toAddress
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
      if (!fromToken || !fromAccount) {
        return
      }
      setEditingFromOrToAmount('from')
      history.replace(
        makeSwapRoute({
          fromToken,
          fromAccount,
          toToken: token,
          toAddress,
          toCoin: token.coin
        })
      )
      setSelectingFromOrTo(undefined)
      setToAmount('')

      reset()
      await handleQuoteRefreshInternal({
        toToken: token,
        toAmount: ''
      })
    },
    [
      fromToken,
      fromAccount,
      history,
      toAddress,
      reset,
      handleQuoteRefreshInternal
    ]
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
      if (!account) {
        return
      }
      setEditingFromOrToAmount('from')
      // ToDo: Until cross-chain swaps is supported,
      // we have this check to make sure that the toToken
      // and the incoming fromToken are on the same network.
      // If not we clear the toToken from params.
      if (toToken && toToken.chainId === token.chainId) {
        history.replace(
          makeSwapRoute({
            fromToken: token,
            fromAccount: account,
            toToken,
            toAddress,
            toCoin
          })
        )
      } else {
        history.replace(
          makeSwapRoute({ fromToken: token, fromAccount: account })
        )
      }
      setSelectingFromOrTo(undefined)
      setFromAmount('')
      setToAmount('')
      reset()
    },
    [toToken, reset, history, toAddress, toCoin]
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

      // EVM specific validations
      if (
        (quoteUnion?.zeroExQuote || quoteUnion?.lifiQuote) &&
        fromToken.coin === BraveWallet.CoinType.ETH &&
        fromToken.contractAddress &&
        !hasAllowance
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

      // LiFi specific validations
      if (
        quoteErrorUnion?.lifiError?.code &&
        quoteErrorUnion?.lifiError?.code !== BraveWallet.LiFiErrorCode.kSuccess
      ) {
        return quoteErrorUnion?.lifiError.code ===
          BraveWallet.LiFiErrorCode.kNotFoundError
          ? 'insufficientLiquidity'
          : 'unknownError'
      }

      return undefined
    }, [
      fromAmount,
      toAmount,
      fromToken,
      toToken,
      fromAssetBalance,
      nativeAssetBalance,
      feesWrapped,
      quoteUnion?.zeroExQuote,
      quoteUnion?.lifiQuote,
      quoteUnion?.jupiterQuote?.routePlan.length,
      hasAllowance,
      quoteErrorUnion
    ])

  const onSubmit = useCallback(async () => {
    if (!quoteUnion || !fromAccount || !fromNetwork || !fromToken) {
      return
    }

    setIsFetchingQuote(true)

    if (quoteUnion.zeroExQuote) {
      if (hasAllowance) {
        const error = await zeroEx.exchange()
        if (error) {
          setQuoteErrorUnion(error)
        } else {
          setFromAmount('')
          setToAmount('')
          reset()
        }
      } else {
        await approveSpendAllowance({
          account: fromAccount,
          network: fromNetwork,
          spenderAddress: quoteUnion.zeroExQuote.allowanceTarget,
          token: fromToken,
          spendAmount: quoteUnion.zeroExQuote.sellAmount
        })
      }
    }

    if (quoteUnion.lifiQuote) {
      const route = quoteUnion.lifiQuote.routes[0] || undefined
      const step = route?.steps[0]

      if (!step) {
        return
      }

      if (hasAllowance) {
        // in the future, we will loop thru the steps and call exchange (await
        // confirmations)
        const error = await lifi.exchange(step)
        if (error) {
          setQuoteErrorUnion(error)
        } else {
          setFromAmount('')
          setToAmount('')
          reset()
        }
      } else {
        await approveSpendAllowance({
          spenderAddress: step.estimate.approvalAddress,
          spendAmount: step.estimate.fromAmount,
          account: fromAccount,
          network: fromNetwork,
          token: fromToken
        })
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
  }, [
    quoteUnion,
    fromAccount,
    fromNetwork,
    fromToken,
    hasAllowance,
    zeroEx,
    reset,
    approveSpendAllowance,
    lifi,
    jupiter
  ])

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

    if (swapValidationError === 'unknownError') {
      return getLocale('braveWalletSwapUnknownError')
    }

    return getLocale('braveSwapReviewOrder')
  }, [fromToken, fromNetwork, swapValidationError])

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
