// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useEffect, useMemo, useState } from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useHistory, useLocation } from 'react-router'

// Options
import { SwapAndSendOptions } from '../../../../options/swap-and-send-options'

// Hooks
import { useJupiter } from './useJupiter'
import { useZeroEx } from './useZeroEx'
import { useLifi } from './useLifi'
import { useSquid } from './useSquid'
import { useGate3 } from './useGate3'
import { useDebouncedCallback } from './useDebouncedCallback'
import {
  useScopedBalanceUpdater, //
} from '../../../../common/hooks/use-scoped-balance-updater'
import { useQuery } from '../../../../common/hooks/use-query'
import {
  useTokenAllowance, //
} from '../../../../common/hooks/use_token_allowance'

// Types and constants
import {
  SwapValidationErrorType,
  SwapParamsOverrides,
  SwapParams,
} from '../constants/types'
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import Amount from '../../../../utils/amount'
// FIXME(onyb): move makeNetworkAsset to utils/assets-utils
import { isNativeAsset } from '../../../../utils/asset-utils'
import { makeNetworkAsset } from '../../../../options/asset-options'
import {
  getPriceRequestsForTokens,
  getTokenPriceAmountFromRegistry,
} from '../../../../utils/pricing-utils'
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
  getLiFiToAmount,
  getSquidFromAmount,
  getSquidToAmount,
  getSquidQuoteOptions,
  getGate3FromAmount,
  getGate3ToAmount,
  getGate3QuoteOptions,
} from '../swap.utils'
import {
  makeSwapOrBridgeRoute, //
} from '../../../../utils/routes-utils'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetSwapSupportedNetworksQuery,
  useGetTokenSpotPricesQuery,
  useGenerateSwapQuoteMutation,
} from '../../../../common/slices/api.slice'
import { querySubscriptionOptions60s } from '../../../../common/slices/constants'
import { AccountInfoEntity } from '../../../../common/slices/entities/account-info.entity'
import { TokenBalancesRegistry } from '../../../../common/slices/entities/token-balance.entity'
import {
  useAccountFromAddressQuery,
  useGetCombinedTokensListQuery,
  useReceiveAddressQuery,
} from '../../../../common/slices/api.slice.extra'

const hasDecimalsOverflow = (
  amount: string,
  asset?: BraveWallet.BlockchainToken,
) => {
  if (!asset) {
    return false
  }

  const amountBaseWrapped = new Amount(amount).multiplyByDecimals(
    asset.decimals,
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
  tokenList: BraveWallet.BlockchainToken[],
) => {
  return tokenList.find(
    (token) =>
      (token.chainId === network.chainId
        && token.coin === network.coin
        && token.contractAddress.toLowerCase()
          === contractOrSymbol.toLowerCase())
      || (token.chainId === network.chainId
        && token.coin === network.coin
        && token.contractAddress === ''
        && token.symbol.toLowerCase() === contractOrSymbol.toLowerCase()),
  )
}

const getAssetBalance = (
  token: BraveWallet.BlockchainToken,
  fromAccount?: BraveWallet.AccountInfo,
  tokenBalancesRegistry?: TokenBalancesRegistry,
): Amount => {
  if (!fromAccount) {
    return Amount.zero()
  }

  return new Amount(
    getBalance(fromAccount.accountId, token, tokenBalancesRegistry),
  )
}

export const useSwap = () => {
  // routing
  const query = useQuery()
  const history = useHistory()
  const { pathname } = useLocation()
  const isBridge = pathname.includes(WalletRoutes.Bridge)

  // Queries
  // FIXME(onyb): what happens when defaultFiatCurrency is empty
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: supportedNetworks } = useGetSwapSupportedNetworksQuery()
  const { data: fullTokenList } = useGetCombinedTokensListQuery()
  const fromAccountIdFromParams = query.get('fromAccountId') ?? undefined
  const { account: fromAccount } = useAccountFromAddressQuery(
    fromAccountIdFromParams,
  )
  const toAccountIdFromParams = query.get('toAccountId') ?? undefined
  const { account: toAccount } = useAccountFromAddressQuery(
    toAccountIdFromParams,
  )
  const toAccountId = toAccount?.accountId ?? undefined

  // For UTXO-based accounts, the address field in the accountId is empty,
  // so we need to use useReceiveAddressQuery to get the actual receive address.
  const {
    receiveAddress: toAccountAddress,
    isFetchingAddress: isFetchingToAccountAddress,
  } = useReceiveAddressQuery(toAccountId)
  const {
    receiveAddress: fromAccountAddress,
    isFetchingAddress: isFetchingFromAccountAddress,
  } = useReceiveAddressQuery(fromAccount?.accountId)
  const needsAddressResolution = useMemo(() => {
    const needsToAddressResolution =
      toAccountId
      && !toAccountId.address
      && (isFetchingToAccountAddress || !toAccountAddress)

    const needsFromAddressResolution =
      fromAccount
      && !fromAccount.accountId.address
      && (isFetchingFromAccountAddress || !fromAccountAddress)

    return needsToAddressResolution || needsFromAddressResolution
  }, [
    toAccountId,
    toAccountAddress,
    isFetchingToAccountAddress,
    fromAccount,
    fromAccountAddress,
    isFetchingFromAccountAddress,
  ])
  const fromContractOrSymbolFromParams = query.get('fromToken') ?? undefined
  const fromChainIdFromParams = query.get('fromChainId') ?? undefined

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
  const [quoteUnion, setQuoteUnion] = useState<
    BraveWallet.SwapQuoteUnion | undefined
  >(undefined)
  const [quoteErrorUnion, setQuoteErrorUnion] = useState<
    BraveWallet.SwapErrorUnion | undefined
  >(undefined)
  const [backendError, setBackendError] = useState<string | undefined>('')
  const [abortController, setAbortController] = useState<
    AbortController | undefined
  >(undefined)
  const [isFetchingQuote, setIsFetchingQuote] = useState<boolean>(false)
  const [swapFees, setSwapFees] = useState<BraveWallet.SwapFees | undefined>(
    undefined,
  )
  const [selectedSwapSendAccount, setSelectedSwapSendAccount] = useState<
    AccountInfoEntity | undefined
  >(undefined)
  const [timeUntilNextQuote, setTimeUntilNextQuote] = useState<
    number | undefined
  >(undefined)
  const [selectedProvider, setSelectedProvider] =
    useState<BraveWallet.SwapProvider>(BraveWallet.SwapProvider.kAuto)
  const [selectedQuoteOptionId, setSelectedQuoteOptionId] = useState<
    string | undefined
  >(undefined)
  const [isSubmittingSwap, setIsSubmittingSwap] = useState<boolean>(false)

  // Mutations
  const [generateSwapQuote] = useGenerateSwapQuoteMutation()

  // Memos
  const fromNetwork = useMemo(() => {
    if (!supportedNetworks?.length) {
      return
    }
    return supportedNetworks.find(
      (network) => network.chainId === fromChainIdFromParams,
    )
  }, [supportedNetworks, fromChainIdFromParams])

  const toNetwork = useMemo(() => {
    if (!supportedNetworks?.length || !toAccountId) {
      return
    }
    return supportedNetworks.find(
      (network) =>
        network.chainId === query.get('toChainId')
        && network.coin === toAccountId.coin,
    )
  }, [supportedNetworks, toAccountId, query])

  const fromToken = useMemo(() => {
    if (!fromContractOrSymbolFromParams || !fromNetwork) {
      return
    }

    return getTokenFromParam(
      fromContractOrSymbolFromParams,
      fromNetwork,
      fullTokenList,
    )
  }, [fullTokenList, fromContractOrSymbolFromParams, fromNetwork])

  const toToken = useMemo(() => {
    const contractOrSymbol = query.get('toToken')
    if (!contractOrSymbol || !toNetwork) {
      return
    }

    return getTokenFromParam(contractOrSymbol, toNetwork, fullTokenList)
  }, [fullTokenList, query, toNetwork])

  const nativeAsset = useMemo(
    () => makeNetworkAsset(fromNetwork),
    [fromNetwork],
  )

  const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
    useScopedBalanceUpdater(
      fromAccount && fromNetwork && fromToken && nativeAsset
        ? {
            network: fromNetwork,
            accounts: [fromAccount],
            tokens: isNativeAsset(fromToken)
              ? [nativeAsset]
              : [nativeAsset, fromToken],
          }
        : skipToken,
    )

  const tokenPriceRequests = useMemo(
    () => getPriceRequestsForTokens([nativeAsset, fromToken, toToken]),
    [nativeAsset, fromToken, toToken],
  )

  const { data: spotPrices = [] } = useGetTokenSpotPricesQuery(
    tokenPriceRequests.length && defaultFiatCurrency
      ? { requests: tokenPriceRequests, vsCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s,
  )

  const quoteOptions = useMemo(() => {
    if (
      !fromNetwork
      || !fromToken
      || !toToken
      || !quoteUnion
      || !spotPrices
      || !defaultFiatCurrency
    ) {
      return []
    }

    if (quoteUnion.lifiQuote) {
      return getLiFiQuoteOptions({
        quote: quoteUnion.lifiQuote,
        fromNetwork,
        spotPrices,
        defaultFiatCurrency,
        toToken,
        fromToken,
      })
    }

    if (quoteUnion.jupiterQuote) {
      return getJupiterQuoteOptions({
        quote: quoteUnion.jupiterQuote,
        fromNetwork,
        fromToken,
        toToken,
        spotPrices,
        defaultFiatCurrency,
      })
    }

    if (quoteUnion.zeroExQuote) {
      return getZeroExQuoteOptions({
        quote: quoteUnion.zeroExQuote,
        fromNetwork,
        fromToken,
        toToken,
        spotPrices,
        defaultFiatCurrency,
      })
    }

    if (quoteUnion.squidQuote) {
      return getSquidQuoteOptions({
        quote: quoteUnion.squidQuote,
        fromNetwork,
        spotPrices,
        defaultFiatCurrency,
      })
    }

    if (quoteUnion.gate3Quote) {
      return getGate3QuoteOptions({
        quote: quoteUnion.gate3Quote,
        fromToken,
        toToken,
        fromNetwork,
        spotPrices,
        defaultFiatCurrency,
      })
    }

    return []
  }, [
    fromNetwork,
    fromToken,
    toToken,
    quoteUnion,
    spotPrices,
    defaultFiatCurrency,
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
      slippageTolerance,
      fromAccountAddress,
      toAccountAddress,
      needsAddressResolution,
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
    slippageTolerance,
    fromAccountAddress,
    toAccountAddress,
    needsAddressResolution,
  ])

  const needsAccountSelected =
    fromAccountIdFromParams === undefined
    && fromContractOrSymbolFromParams !== undefined
    && fromChainIdFromParams !== undefined

  const jupiter = useJupiter(swapProviderHookParams)
  const zeroEx = useZeroEx(swapProviderHookParams)
  const lifi = useLifi(swapProviderHookParams)
  const squid = useSquid(swapProviderHookParams)
  const gate3 = useGate3(swapProviderHookParams)
  const { approveSpendAllowance, checkAllowance, hasAllowance } =
    useTokenAllowance()

  const reset = useCallback(() => {
    setQuoteUnion(undefined)
    setQuoteErrorUnion(undefined)
    setBackendError(undefined)
    setIsFetchingQuote(false)
    setSwapFees(undefined)
    setTimeUntilNextQuote(undefined)
    setSelectedQuoteOptionId(undefined)
    if (abortController) {
      abortController.abort()
    }
  }, [abortController])

  const onSelectQuoteOption = useCallback(
    (id?: string) => {
      const option = id
        ? quoteOptions.find((option) => option.id === id)
        : quoteOptions[0]
      if (!option) {
        return
      }

      setSelectedQuoteOptionId(id)
      setToAmount(option.toAmount.format(6))
    },
    [quoteOptions],
  )

  const fromAssetBalance = useMemo(
    () =>
      fromToken
      && getAssetBalance(fromToken, fromAccount, tokenBalancesRegistry),
    [fromToken, fromAccount, tokenBalancesRegistry],
  )
  const nativeAssetBalance = useMemo(
    () =>
      nativeAsset
      && getAssetBalance(nativeAsset, fromAccount, tokenBalancesRegistry),
    [nativeAsset, fromAccount, tokenBalancesRegistry],
  )

  const handleQuoteRefreshInternal = useCallback(
    async (overrides: SwapParamsOverrides) => {
      const effectiveToAccountId = overrides.toAccountId ?? toAccountId
      if (
        !fromAccount
        || !effectiveToAccountId
        || !fromNetwork
        || !fromAssetBalance
      ) {
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
        toAccountId: effectiveToAccountId,
        editingFromOrToAmount,
        provider:
          overrides.provider === undefined
            ? selectedProvider
            : overrides.provider,
        slippage: overrides.slippage ?? slippageTolerance,
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
        fromAmountWrapped.isZero()
        || fromAmountWrapped.isNaN()
        || fromAmountWrapped.isUndefined()
      const isToAmountEmpty =
        toAmountWrapped.isZero()
        || toAmountWrapped.isNaN()
        || toAmountWrapped.isUndefined()

      if (isFromAmountEmpty && isToAmountEmpty) {
        reset()
        return
      }

      // For UTXO accounts (like BTC), wait for the receive addresses to be
      // fetched.
      if (needsAddressResolution) {
        return
      }

      const controller = new AbortController()
      setAbortController(controller)
      setIsFetchingQuote(true)
      setQuoteErrorUnion(undefined)
      setBackendError(undefined)

      let quoteResponse
      try {
        quoteResponse = await generateSwapQuote({
          fromAccountId: {
            ...fromAccount.accountId,
            // Use fetched address for UTXO accounts where address field is empty
            address: fromAccount.accountId.address || fromAccountAddress || '',
          },
          fromChainId: params.fromToken.chainId,
          fromAmount:
            params.editingFromOrToAmount === 'from' && params.fromAmount
              ? new Amount(params.fromAmount)
                  .multiplyByDecimals(params.fromToken.decimals)
                  .format()
              : '',
          fromToken: params.fromToken.contractAddress,
          toAccountId: params.toAccountId && {
            ...params.toAccountId,
            // Use fetched address for UTXO accounts where address field is empty
            address: params.toAccountId.address || toAccountAddress || '',
          },
          toChainId: params.toToken.chainId,
          toAmount:
            params.editingFromOrToAmount === 'to' && params.toAmount
              ? new Amount(params.toAmount)
                  .multiplyByDecimals(params.toToken.decimals)
                  .format()
              : '',
          toToken: params.toToken.contractAddress,
          slippagePercentage: params.slippage,
          routePriority: BraveWallet.RoutePriority.kCheapest,
          provider: params.provider,
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

      if (quoteResponse?.errorString) {
        setBackendError(quoteResponse.errorString)
      }

      if (quoteResponse?.response) {
        setQuoteUnion(quoteResponse.response)

        if (quoteResponse.response.lifiQuote) {
          const { routes } = quoteResponse.response.lifiQuote

          // If overrides.selectedQuoteOptionId is undefined, we will use the
          // first route as the default option.
          //
          // If overrides.selectedQuoteOptionId is set, we will try to find the
          // route that matches the selectedQuoteOptionId.
          const route = overrides.selectedQuoteOptionId
            ? routes.find(
                (route) => route.uniqueId === overrides.selectedQuoteOptionId,
              ) || routes[0]
            : routes[0]

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
              spendAmount: fromAssetBalance.format(),
              spenderAddress: step.estimate.approvalAddress,
              token: params.fromToken,
            })
          }
        }

        if (quoteResponse.response.jupiterQuote) {
          if (params.editingFromOrToAmount === 'from') {
            setToAmount(
              getJupiterToAmount({
                quote: quoteResponse.response.jupiterQuote,
                toToken: params.toToken,
              }).format(6),
            )
          } else {
            setFromAmount(
              getJupiterFromAmount({
                quote: quoteResponse.response.jupiterQuote,
                fromToken: params.fromToken,
              }).format(6),
            )
          }
        }

        if (quoteResponse.response.zeroExQuote) {
          if (params.editingFromOrToAmount === 'from') {
            setToAmount(
              getZeroExToAmount({
                quote: quoteResponse.response.zeroExQuote,
                toToken: params.toToken,
              }).format(6),
            )
          } else {
            setFromAmount(
              getZeroExFromAmount({
                quote: quoteResponse.response.zeroExQuote,
                fromToken: params.fromToken,
              }).format(6),
            )
          }

          await checkAllowance({
            account: fromAccount,
            spendAmount: fromAssetBalance.format(),
            spenderAddress: quoteResponse.response.zeroExQuote.allowanceTarget,
            token: params.fromToken,
          })
        }

        if (quoteResponse.response.squidQuote) {
          if (params.editingFromOrToAmount === 'from') {
            setToAmount(
              getSquidToAmount({
                quote: quoteResponse.response.squidQuote,
                toToken: params.toToken,
              }).format(6),
            )
          } else {
            setFromAmount(
              getSquidFromAmount({
                quote: quoteResponse.response.squidQuote,
                fromToken: params.fromToken,
              }).format(6),
            )
          }

          await checkAllowance({
            account: fromAccount,
            spendAmount: fromAssetBalance.format(),
            spenderAddress: quoteResponse.response.squidQuote.allowanceTarget,
            token: params.fromToken,
          })
        }

        if (quoteResponse.response.gate3Quote) {
          const { routes } = quoteResponse.response.gate3Quote

          // If overrides.selectedQuoteOptionId is undefined, we will use the
          // first route as the default option.
          const route = overrides.selectedQuoteOptionId
            ? routes.find(
                (route) => route.id === overrides.selectedQuoteOptionId,
              ) || routes[0]
            : routes[0]

          if (!route) {
            return
          }

          if (params.editingFromOrToAmount === 'from') {
            setToAmount(
              getGate3ToAmount({
                route,
                toToken: params.toToken,
              }).format(6),
            )
          } else {
            setFromAmount(
              getGate3FromAmount({
                route,
                fromToken: params.fromToken,
              }).format(6),
            )
          }

          // Check allowance for EVM tokens that require it
          if (
            route.requiresTokenAllowance
            && params.fromToken.coin === BraveWallet.CoinType.ETH
            && route.depositAddress
          ) {
            await checkAllowance({
              account: fromAccount,
              spendAmount: fromAssetBalance.format(),
              spenderAddress: route.depositAddress,
              token: params.fromToken,
            })
          }
        }
      }

      setSelectedQuoteOptionId(overrides.selectedQuoteOptionId)

      if (quoteResponse?.fees) {
        setSwapFees(quoteResponse?.fees)
      }

      setIsFetchingQuote(false)
      setAbortController(undefined)
      setTimeUntilNextQuote(30000)
    },
    [
      fromAccount,
      fromAccountAddress,
      toAccountId,
      toAccountAddress,
      needsAddressResolution,
      fromNetwork,
      fromAmount,
      toAmount,
      fromToken,
      toToken,
      fromAssetBalance,
      editingFromOrToAmount,
      reset,
      generateSwapQuote,
      slippageTolerance,
      checkAllowance,
      selectedProvider,
    ],
  )

  const handleQuoteRefresh = useDebouncedCallback(
    async (overrides: SwapParamsOverrides) => {
      await handleQuoteRefreshInternal(overrides)
    },
    700,
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
        toAmount: '',
      })
    },
    [handleQuoteRefresh],
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
        toAmount: value,
      })
    },
    [handleQuoteRefresh],
  )

  const onClickFlipSwapTokens = useCallback(async () => {
    if (!fromAccount || !toAccount || !fromToken || !toToken) {
      return
    }
    if (
      !isBridge
      && (fromAccount.accountId.coin !== toToken.coin
        || toAccountId?.coin !== fromToken.coin)
    ) {
      history.replace(WalletRoutes.Swap)
    } else {
      history.replace(
        makeSwapOrBridgeRoute({
          fromToken: toToken,
          fromAccount: toAccount,
          toToken: fromToken,
          toAccountId: fromAccount.accountId,
          routeType: isBridge ? 'bridge' : 'swap',
        }),
      )
    }
    await handleOnSetFromAmount('')
  }, [
    fromAccount,
    toAccount,
    fromToken,
    toToken,
    toAccountId,
    handleOnSetFromAmount,
    history,
    isBridge,
  ])

  // Changing the To asset does the following:
  //  1. Update toToken right away.
  //  2. Reset toAmount.
  //  3. Reset the previously displayed quote, if any.
  //  4. Fetch new quotes using fromAmount and fromToken, if set. Do NOT use
  //     debouncing.
  //  5. Fetch spot price.
  const onSelectToToken = useCallback(
    async (
      token: BraveWallet.BlockchainToken,
      account?: BraveWallet.AccountInfo,
    ) => {
      if (!fromToken || !fromAccount) {
        return
      }
      setEditingFromOrToAmount('from')
      history.replace(
        makeSwapOrBridgeRoute({
          fromToken,
          fromAccount,
          toToken: token,
          toAccountId: account?.accountId,
          routeType: isBridge ? 'bridge' : 'swap',
        }),
      )
      setSelectingFromOrTo(undefined)
      setToAmount('')

      reset()
      await handleQuoteRefreshInternal({
        toToken: token,
        toAmount: '',
        toAccountId: account?.accountId,
      })
    },
    [
      fromToken,
      fromAccount,
      history,
      isBridge,
      reset,
      handleQuoteRefreshInternal,
    ],
  )

  // Changing the From asset does the following:
  //  1. Reset both fromAmount and toAmount.
  //  2. Reset the previously displayed quote, if any.
  //  3. Refresh balance for the new fromToken.
  //  4. Refresh spot price.
  const onSelectFromToken = useCallback(
    async (
      token: BraveWallet.BlockchainToken,
      account?: BraveWallet.AccountInfo,
    ) => {
      if (!account) {
        return
      }
      setEditingFromOrToAmount('from')

      if (isBridge) {
        history.replace(
          makeSwapOrBridgeRoute({
            fromToken: token,
            fromAccount: account,
            toToken,
            toAccountId,
            routeType: 'bridge',
          }),
        )
        setSelectingFromOrTo(undefined)
        setFromAmount('')
        setToAmount('')
        reset()
        return
      }

      // For regular Swaps we check that the toToken
      // and the incoming fromToken are on the same network.
      // If not we clear the toToken from params.
      if (toToken && toToken.chainId === token.chainId) {
        history.replace(
          makeSwapOrBridgeRoute({
            fromToken: token,
            fromAccount: account,
            toToken,
            toAccountId,
            routeType: 'swap',
          }),
        )
      } else {
        history.replace(
          makeSwapOrBridgeRoute({
            fromToken: token,
            fromAccount: account,
            routeType: 'swap',
          }),
        )
      }
      setSelectingFromOrTo(undefined)
      setFromAmount('')
      setToAmount('')
      reset()
    },
    [toToken, reset, history, toAccountId, isBridge],
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
    [],
  )

  const onChangeRecipient = useCallback(
    async (address: string, account?: BraveWallet.AccountInfo) => {
      if (!fromToken || !fromAccount || !toToken) {
        return
      }
      history.replace(
        makeSwapOrBridgeRoute({
          fromToken,
          fromAccount,
          toToken: toToken,
          toAccountId: account?.accountId,
          routeType: isBridge ? 'bridge' : 'swap',
        }),
      )

      // Refresh quote with the new recipient account
      await handleQuoteRefreshInternal({
        toAccountId: account?.accountId,
      })
    },
    [
      fromToken,
      toToken,
      fromAccount,
      history,
      isBridge,
      handleQuoteRefreshInternal,
    ],
  )

  // Memos
  const fiatValue = useMemo(() => {
    if (!fromAmount || !fromToken || !spotPrices || !defaultFiatCurrency) {
      return
    }

    const price = getTokenPriceAmountFromRegistry(spotPrices, fromToken)
    return new Amount(fromAmount).times(price).formatAsFiat(defaultFiatCurrency)
  }, [fromAmount, fromToken, spotPrices, defaultFiatCurrency])

  const feesWrapped = useMemo(() => {
    if (quoteOptions.length) {
      return quoteOptions[0].networkFee
    }

    return Amount.zero()
  }, [quoteOptions])

  const availableProvidersForSwap = useMemo(() => {
    // If no tokens are selected, we cannot reliably determine the available
    // providers.
    if (!fromToken && !toToken) {
      return [BraveWallet.SwapProvider.kAuto]
    }

    const hasSolInFillPath =
      fromToken?.coin === BraveWallet.CoinType.SOL
      || toToken?.coin === BraveWallet.CoinType.SOL

    const hasEthInFillPath =
      fromToken?.coin === BraveWallet.CoinType.ETH
      || toToken?.coin === BraveWallet.CoinType.ETH

    if (!isBridge && hasSolInFillPath) {
      return [BraveWallet.SwapProvider.kAuto, BraveWallet.SwapProvider.kJupiter]
    }

    if (!isBridge && hasEthInFillPath) {
      return [
        BraveWallet.SwapProvider.kAuto,
        BraveWallet.SwapProvider.kLiFi,
        BraveWallet.SwapProvider.kZeroEx,
        BraveWallet.SwapProvider.kSquid,
      ]
    }

    if (isBridge && hasSolInFillPath) {
      return [
        BraveWallet.SwapProvider.kAuto,
        BraveWallet.SwapProvider.kLiFi,
        BraveWallet.SwapProvider.kNearIntents,
      ]
    }

    if (isBridge && hasEthInFillPath) {
      return [
        BraveWallet.SwapProvider.kAuto,
        BraveWallet.SwapProvider.kLiFi,
        BraveWallet.SwapProvider.kSquid,
        BraveWallet.SwapProvider.kNearIntents,
      ]
    }

    return [BraveWallet.SwapProvider.kAuto]
  }, [isBridge, fromToken, toToken])

  const swapValidationError: SwapValidationErrorType | undefined =
    useMemo(() => {
      // Display error if provider is not supported for a swap.
      if (!availableProvidersForSwap.includes(selectedProvider)) {
        return 'providerNotSupported'
      }

      // No validation to perform when From and To amounts
      // are empty, since quote is not fetched.
      if (!fromAmount && !toAmount) {
        return
      }

      if (
        fromToken
        && fromAmount
        && hasDecimalsOverflow(fromAmount, fromToken)
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

      // No quote-based validations to perform when backend error is set.
      if (backendError) {
        return 'unknownError'
      }

      // 0x specific validations
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
        quoteErrorUnion?.lifiError?.code
        && quoteErrorUnion?.lifiError?.code
          !== BraveWallet.LiFiErrorCode.kSuccess
      ) {
        return quoteErrorUnion?.lifiError.code
          === BraveWallet.LiFiErrorCode.kNotFoundError
          ? 'insufficientLiquidity'
          : 'unknownError'
      }

      if (quoteErrorUnion?.squidError) {
        if (quoteErrorUnion.squidError.isInsufficientLiquidity) {
          return 'insufficientLiquidity'
        }

        return 'unknownError'
      }

      // Gate3 specific validations
      if (quoteErrorUnion?.gate3Error) {
        if (
          quoteErrorUnion.gate3Error.kind
          === BraveWallet.Gate3SwapErrorKind.kInsufficientLiquidity
        ) {
          return 'insufficientLiquidity'
        }

        return 'unknownError'
      }

      if (quoteUnion?.gate3Quote?.routes.length === 0) {
        return 'insufficientLiquidity'
      }

      const fromAmountWeiWrapped = new Amount(fromAmount).multiplyByDecimals(
        fromToken.decimals,
      )
      if (fromAmountWeiWrapped.gt(fromAssetBalance)) {
        return 'insufficientBalance'
      }

      if (feesWrapped.gt(nativeAssetBalance)) {
        return 'insufficientFundsForGas'
      }

      if (
        !fromToken.contractAddress
        && fromAmountWeiWrapped.plus(feesWrapped).gt(fromAssetBalance)
      ) {
        return 'insufficientFundsForGas'
      }

      // EVM specific validations
      if (
        (quoteUnion?.zeroExQuote
          || quoteUnion?.lifiQuote
          || quoteUnion?.squidQuote)
        && fromToken.coin === BraveWallet.CoinType.ETH
        && fromToken.contractAddress
        && !hasAllowance
      ) {
        return 'insufficientAllowance'
      }

      // Gate3 EVM allowance check
      if (quoteUnion?.gate3Quote) {
        const route = selectedQuoteOptionId
          ? quoteUnion.gate3Quote.routes.find(
              (r) => r.id === selectedQuoteOptionId,
            ) || quoteUnion.gate3Quote.routes[0]
          : quoteUnion.gate3Quote.routes[0]

        if (
          route?.requiresTokenAllowance
          && fromToken.coin === BraveWallet.CoinType.ETH
          && fromToken.contractAddress
          && !hasAllowance
        ) {
          return 'insufficientAllowance'
        }
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
      quoteUnion?.squidQuote,
      quoteUnion?.gate3Quote,
      hasAllowance,
      quoteErrorUnion,
      backendError,
      selectedProvider,
      availableProvidersForSwap,
      selectedQuoteOptionId,
    ])

  const onSubmit = useCallback(async () => {
    if (
      !quoteUnion
      || !fromAccount
      || !fromNetwork
      || !fromToken
      || !fromAssetBalance
    ) {
      return
    }

    setIsSubmittingSwap(true)

    if (quoteUnion.zeroExQuote) {
      if (hasAllowance) {
        const error = await zeroEx.exchange()
        if (error) {
          console.log('zeroEx.exchange error', error.zeroExError)
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
          spendAmount: fromAssetBalance.format(),
        })
      }
    }

    if (quoteUnion.lifiQuote) {
      const route = selectedQuoteOptionId
        ? quoteUnion.lifiQuote.routes.find(
            (route) => route.uniqueId === selectedQuoteOptionId,
          ) || quoteUnion.lifiQuote.routes[0]
        : quoteUnion.lifiQuote.routes[0]

      const step = route?.steps[0]
      if (!step) {
        return
      }

      if (hasAllowance) {
        // in the future, we will loop thru the steps and call exchange (await
        // confirmations)
        const error = await lifi.exchange(step)
        if (error) {
          console.log('lifi.exchange error', error.lifiError)
          setQuoteErrorUnion(error)
        } else {
          setFromAmount('')
          setToAmount('')
          reset()
        }
      } else {
        await approveSpendAllowance({
          spenderAddress: step.estimate.approvalAddress,
          spendAmount: fromAssetBalance.format(),
          account: fromAccount,
          network: fromNetwork,
          token: fromToken,
        })
      }
    }

    if (quoteUnion.jupiterQuote) {
      const error = await jupiter.exchange(quoteUnion.jupiterQuote)
      if (error) {
        console.log('jupiter.exchange error', error.jupiterError)
        setQuoteErrorUnion(error)
      } else {
        setFromAmount('')
        setToAmount('')
        reset()
      }
    }

    if (quoteUnion.squidQuote) {
      if (hasAllowance) {
        const error = await squid.exchange()
        if (error) {
          console.log('squid.exchange error', error.squidError)
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
          spenderAddress: quoteUnion.squidQuote.allowanceTarget,
          token: fromToken,
          spendAmount: fromAssetBalance.format(),
        })
      }
    }

    if (quoteUnion.gate3Quote) {
      const route = selectedQuoteOptionId
        ? quoteUnion.gate3Quote.routes.find(
            (route) => route.id === selectedQuoteOptionId,
          ) || quoteUnion.gate3Quote.routes[0]
        : quoteUnion.gate3Quote.routes[0]

      if (!route) {
        setIsSubmittingSwap(false)
        return
      }

      // Check if we need to approve allowance first
      if (
        route.requiresTokenAllowance
        && fromToken.coin === BraveWallet.CoinType.ETH
        && !hasAllowance
      ) {
        if (route.depositAddress) {
          await approveSpendAllowance({
            account: fromAccount,
            network: fromNetwork,
            spenderAddress: route.depositAddress,
            token: fromToken,
            spendAmount: fromAssetBalance.format(),
          })
        }
      } else {
        const error = await gate3.exchange(route)
        if (error) {
          console.log('gate3.exchange error', error.gate3Error)
          setQuoteErrorUnion(error)
        } else {
          setFromAmount('')
          setToAmount('')
          reset()
        }
      }
    }

    setIsSubmittingSwap(false)
  }, [
    selectedQuoteOptionId,
    quoteUnion,
    fromAccount,
    fromNetwork,
    fromToken,
    fromAssetBalance,
    hasAllowance,
    zeroEx,
    reset,
    approveSpendAllowance,
    lifi,
    jupiter,
    squid,
    gate3,
  ])

  const onChangeSwapProvider = useCallback(
    async (provider: BraveWallet.SwapProvider) => {
      setSelectedProvider(provider)
      setQuoteErrorUnion(undefined)
      setBackendError(undefined)
      setQuoteUnion(undefined)
      setSelectedQuoteOptionId(undefined)
      await handleQuoteRefreshInternal({
        provider,
      })
    },
    [handleQuoteRefreshInternal],
  )

  const onChangeSlippageTolerance = useCallback(
    async (slippage: string) => {
      setSlippageTolerance(slippage)
      await handleQuoteRefreshInternal({
        slippage,
      })
    },
    [handleQuoteRefreshInternal],
  )

  const submitButtonText = useMemo(() => {
    if (isFetchingQuote) {
      return getLocale('braveWalletFetchingQuote')
    }

    const defaultText = isBridge
      ? getLocale('braveWalletReviewBridge')
      : getLocale('braveWalletReviewSwap')

    if (!fromToken || !fromNetwork) {
      return defaultText
    }

    if (swapValidationError === 'insufficientBalance') {
      return getLocale('braveSwapInsufficientBalance').replace(
        '$1',
        fromToken.symbol,
      )
    }

    if (swapValidationError === 'insufficientFundsForGas') {
      return getLocale('braveSwapInsufficientBalance').replace(
        '$1',
        fromNetwork.symbol,
      )
    }

    if (
      fromNetwork.coin === BraveWallet.CoinType.ETH
      && swapValidationError === 'insufficientAllowance'
    ) {
      return getLocale('braveSwapApproveToken').replace('$1', fromToken.symbol)
    }

    if (swapValidationError === 'insufficientLiquidity') {
      return getLocale('braveSwapInsufficientLiquidity')
    }

    if (swapValidationError === 'unknownError') {
      return getLocale('braveWalletSwapUnknownError')
    }

    return defaultText
  }, [isBridge, fromToken, fromNetwork, swapValidationError, isFetchingQuote])

  const isSubmitButtonDisabled = useMemo(() => {
    return (
      !fromNetwork
      || !fromAccount
      || !networkSupportsAccount(fromNetwork, fromAccount.accountId)
      || !toNetwork
      // Prevent creating a swap transaction with stale parameters if fetching
      // of a new quote is in progress.
      || isFetchingQuote
      || isSubmittingSwap
      // For UTXO accounts (like BTC), wait for the receive addresses to be
      // fetched.
      || needsAddressResolution
      // If quote is not set, there's nothing to create the swap with, so Swap
      // button must be disabled.
      || quoteUnion === undefined
      // FROM/TO assets may be undefined during initialization of the swap
      // assets list.
      || fromToken === undefined
      || toToken === undefined
      // Amounts must be defined by the user, or populated from the swap quote,
      // for creating a transaction.
      || new Amount(toAmount).isUndefined()
      || new Amount(toAmount).isZero()
      || new Amount(fromAmount).isUndefined()
      || new Amount(fromAmount).isZero()
      // Disable Swap button if native asset balance has not been fetched yet,
      // to ensure insufficientFundsForGas error (if applicable) is accurate.
      || nativeAssetBalance === undefined
      // Disable Swap button if FROM asset balance has not been fetched yet,
      // to ensure insufficientBalance error (if applicable) is accurate.
      || fromAssetBalance === undefined
      // Unless the validation error is insufficientAllowance, in which case
      // the transaction is an ERC20Approve, Swap button must be disabled.
      || (swapValidationError
        && fromNetwork.coin === BraveWallet.CoinType.ETH
        && swapValidationError !== 'insufficientAllowance')
      || (swapValidationError && fromNetwork.coin === BraveWallet.CoinType.SOL)
    )
  }, [
    fromNetwork,
    fromAccount,
    toNetwork,
    isFetchingQuote,
    needsAddressResolution,
    quoteUnion,
    fromToken,
    toToken,
    toAmount,
    fromAmount,
    nativeAssetBalance,
    fromAssetBalance,
    swapValidationError,
    isSubmittingSwap,
  ])

  const handleQuoteRefreshWithSelectedQuoteOptionId = useCallback(async () => {
    await handleQuoteRefresh({
      selectedQuoteOptionId,
    })
  }, [handleQuoteRefresh, selectedQuoteOptionId])

  useEffect(() => {
    const interval = setInterval(async () => {
      if (timeUntilNextQuote && timeUntilNextQuote !== 0) {
        setTimeUntilNextQuote(timeUntilNextQuote - 1000)
        return
      }
      if (!isFetchingQuote) {
        await handleQuoteRefreshWithSelectedQuoteOptionId()
      }
    }, 1000)
    return () => {
      clearInterval(interval)
    }
  }, [
    handleQuoteRefreshWithSelectedQuoteOptionId,
    timeUntilNextQuote,
    isFetchingQuote,
    selectedQuoteOptionId,
  ])

  useEffect(() => {
    // Reset selectedProvider to Auto if no tokens are selected
    if (!fromToken && !toToken) {
      setFromAmount('')
      setToAmount('')
      reset()
      setSelectedProvider(BraveWallet.SwapProvider.kAuto)
    }
  }, [fromToken, toToken, reset])

  useEffect(() => {
    if (needsAccountSelected) {
      setSelectingFromOrTo('from')
    }
  }, [needsAccountSelected])

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
    selectedQuoteOptionId,
    selectingFromOrTo,
    swapAndSendSelected,
    selectedSwapAndSendOption,
    selectedSwapSendAccount,
    toAnotherAddress,
    userConfirmedAddress,
    slippageTolerance,
    useDirectRoute,
    swapFees,
    onSelectFromToken,
    onSelectToToken,
    onSelectQuoteOption,
    setSelectingFromOrTo,
    handleOnSetFromAmount,
    handleOnSetToAmount,
    handleQuoteRefresh: handleQuoteRefreshWithSelectedQuoteOptionId,
    onClickFlipSwapTokens,
    setSwapAndSendSelected,
    handleOnSetToAnotherAddress,
    onCheckUserConfirmedAddress,
    onSetSelectedSwapAndSendOption,
    setSelectedSwapSendAccount,
    onChangeSlippageTolerance,
    setUseDirectRoute,
    onSubmit,
    onChangeRecipient,
    onChangeSwapProvider,
    submitButtonText,
    isSubmitButtonDisabled,
    swapValidationError,
    spotPrices,
    tokenBalancesRegistry,
    isLoadingBalances,
    isBridge,
    toAccount,
    timeUntilNextQuote,
    selectedProvider,
    availableProvidersForSwap,
    isSubmittingSwap,
    needsAccountSelected,
  }
}
export default useSwap
