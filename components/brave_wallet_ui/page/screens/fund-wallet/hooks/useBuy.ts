// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useState, useEffect, useCallback, useMemo } from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/dist/query'

// Types
import {
  MeldCryptoCurrency,
  MeldFiatCurrency,
  BraveWallet,
  MeldCryptoQuote,
  MeldPaymentMethod,
  CryptoBuySessionData,
  CryptoWidgetCustomerData,
  WalletRoutes,
} from '../../../../constants/types'

// Hooks
import {
  useDebouncedCallback, //
} from '../../swap/hooks/useDebouncedCallback'
import { useQuery } from '../../../../common/hooks/use-query'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetMeldFiatCurrenciesQuery,
  useGetMeldCryptoCurrenciesQuery,
  useGetMeldCountriesQuery,
  useGenerateMeldCryptoQuotesMutation,
  useGetTokenSpotPricesQuery,
  useGetMeldServiceProvidersQuery,
  useGetMeldPaymentMethodsQuery,
  useCreateMeldBuyWidgetMutation,
} from '../../../../common/slices/api.slice'
import {
  useAccountFromAddressQuery,
  useAccountsQuery,
  useReceiveAddressQuery,
} from '../../../../common/slices/api.slice.extra'

// Constants
import { querySubscriptionOptions60s } from '../../../../common/slices/constants'

// Utils
import Amount from '../../../../utils/amount'
import {
  getAssetSymbol,
  getMeldTokensChainId,
  getMeldTokensCoinType,
} from '../../../../utils/meld_utils'
import { makeFundWalletRoute } from '../../../../utils/routes-utils'
import {
  getPriceRequestsForTokens,
  getTokenPriceAmountFromRegistry,
} from '../../../../utils/pricing-utils'

export type BuyParamOverrides = {
  country?: string
  paymentMethod?: MeldPaymentMethod
  sourceCurrencyCode?: string
  destinationCurrencyCode?: string
  amount?: string
  receiveAddress?: string
}

const DEFAULT_ASSET: MeldCryptoCurrency = {
  'currencyCode': 'ETH',
  'name': 'Ethereum',
  'chainCode': 'ETH',
  'chainName': 'Ethereum',
  'chainId': '1',
  'contractAddress': '0x0000000000000000000000000000000000000000',
  'symbolImageUrl': 'https://images-currency.meld.io/crypto/ETH/symbol.png',
}

const DEFAULT_PAYMENT_METHOD: MeldPaymentMethod = {
  logoImages: {
    darkShortUrl: '',
    darkUrl:
      'https://images-paymentMethod.meld.io/CREDIT_DEBIT_CARD/logo_dark.png',
    lightShortUrl: '',
    lightUrl:
      'https://images-paymentMethod.meld.io/CREDIT_DEBIT_CARD/logo_light.png',
  },
  name: 'Credit & Debit Card',
  paymentMethod: 'CREDIT_DEBIT_CARD',
  paymentType: 'CARD',
}

const getFirstAccountByCoinType = (
  coin: BraveWallet.CoinType,
  accounts: BraveWallet.AccountInfo[],
) => {
  return accounts.filter((account) => account.accountId.coin === coin)[0]
}

export const useBuy = () => {
  // Routing
  const history = useHistory()
  const query = useQuery()

  // Queries
  const { data: defaultFiatCurrency = 'USD' } = useGetDefaultFiatCurrencyQuery()
  const { data: fiatCurrencies } = useGetMeldFiatCurrenciesQuery()
  const { data: meldSupportedBuyAssets, isLoading: isLoadingAssets } =
    useGetMeldCryptoCurrenciesQuery()
  const { accounts } = useAccountsQuery()
  const { data: countries, isLoading: isLoadingCountries } =
    useGetMeldCountriesQuery()
  const { data: serviceProviders = [], isLoading: isLoadingServiceProvider } =
    useGetMeldServiceProvidersQuery()
  const { account: accountFromParams } = useAccountFromAddressQuery(
    query.get('accountId') ?? undefined,
  )
  const chainId = query.get('chainId') ?? undefined
  const currencyCode = query.get('currencyCode') ?? undefined

  // State
  const [selectedCurrency, setSelectedCurrency] = useState<
    MeldFiatCurrency | undefined
  >(undefined)
  const [amount, setAmount] = useState<string>('100')
  const [abortController, setAbortController] = useState<
    AbortController | undefined
  >(undefined)
  const [isFetchingQuotes, setIsFetchingQuotes] = useState(false)
  const [hasQuoteError, setHasQuoteError] = useState<boolean>(false)
  const [quotes, setQuotes] = useState<MeldCryptoQuote[]>([])
  const [timeUntilNextQuote, setTimeUntilNextQuote] = useState<
    number | undefined
  >(undefined)
  const [selectedCountryCode, setSelectedCountryCode] = useState<string>('US')
  const [selectedPaymentMethod, setSelectedPaymentMethod] =
    useState<MeldPaymentMethod>(DEFAULT_PAYMENT_METHOD)
  const [isCreatingWidgetFor, setIsCreatingWidgetFor] = useState<
    string | undefined
  >(undefined)
  const [searchTerm, setSearchTerm] = useState<string>('')
  const [showCreateAccount, setShowCreateAccount] = useState<boolean>(false)
  const [pendingSelectedToken, setPendingSelectedToken] = useState<
    MeldCryptoCurrency | undefined
  >(undefined)

  // Mutations
  const [generateQuotes] = useGenerateMeldCryptoQuotesMutation()
  const [createMeldBuyWidget] = useCreateMeldBuyWidgetMutation()
  const { data: paymentMethods, isLoading: isLoadingPaymentMethods } =
    useGetMeldPaymentMethodsQuery(
      selectedCountryCode && defaultFiatCurrency
        ? {
            country: selectedCountryCode,
            sourceCurrencyCode: defaultFiatCurrency,
          }
        : skipToken,
    )

  // Memos and Queries
  const selectedMeldAsset = useMemo(() => {
    if (!currencyCode || !meldSupportedBuyAssets || !chainId) {
      return DEFAULT_ASSET
    }

    return (
      meldSupportedBuyAssets.find(
        (asset) =>
          asset.currencyCode === currencyCode && asset.chainId === chainId,
      ) ?? DEFAULT_ASSET
    )
  }, [meldSupportedBuyAssets, currencyCode, chainId])

  const selectedAsset: Pick<
    BraveWallet.BlockchainToken,
    'coin' | 'chainId' | 'contractAddress'
  > = useMemo(
    () => ({
      coin: getMeldTokensCoinType(selectedMeldAsset),
      chainId: getMeldTokensChainId(selectedMeldAsset) ?? '',
      contractAddress: selectedMeldAsset.contractAddress || '',
    }),
    [selectedMeldAsset],
  )

  const selectedAssetPriceRequests = useMemo(
    () => getPriceRequestsForTokens([selectedAsset]),
    [selectedAsset],
  )

  const { data: spotPrices = [] } = useGetTokenSpotPricesQuery(
    selectedAssetPriceRequests.length && selectedCurrency?.currencyCode
      ? {
          requests: selectedAssetPriceRequests,
          vsCurrency: selectedCurrency?.currencyCode || defaultFiatCurrency,
        }
      : skipToken,
    querySubscriptionOptions60s,
  )

  const selectedAccount = useMemo(() => {
    if (!accountFromParams) {
      return getFirstAccountByCoinType(selectedAsset.coin, accounts)
    }
    return accountFromParams
  }, [accountFromParams, accounts, selectedAsset])

  const selectedAssetSpotPrice = useMemo(() => {
    if (selectedAsset && spotPrices) {
      return getTokenPriceAmountFromRegistry(spotPrices, selectedAsset)
    }
    return undefined
  }, [selectedAsset, spotPrices])

  const [cryptoEstimate, formattedCryptoEstimate] = useMemo(() => {
    if (selectedAssetSpotPrice && selectedMeldAsset) {
      const symbol = getAssetSymbol(selectedMeldAsset)
      const estimate = new Amount(amount).div(selectedAssetSpotPrice)

      return [
        estimate?.toNumber().toString(),
        estimate?.formatAsAsset(5, symbol),
      ]
    }
    return ['', '']
  }, [selectedAssetSpotPrice, selectedMeldAsset, amount])

  const quotesSortedByBestReturn = useMemo(() => {
    if (quotes.length === 0) {
      return []
    }
    return Array.from(quotes).sort(function (a, b) {
      return new Amount(b.destinationAmount ?? '0')
        .minus(a.destinationAmount ?? '0')
        .toNumber()
    })
  }, [quotes])

  const filteredQuotes = useMemo(() => {
    if (searchTerm === '') {
      return quotesSortedByBestReturn
    }

    return quotesSortedByBestReturn.filter(
      (quote) =>
        quote.serviceProvider
          ?.toLowerCase()
          .startsWith(searchTerm.toLowerCase())
        || quote.serviceProvider
          ?.toLowerCase()
          .includes(searchTerm.toLowerCase()),
    )
  }, [quotesSortedByBestReturn, searchTerm])

  const { receiveAddress } = useReceiveAddressQuery(selectedAccount?.accountId)

  // Methods
  const reset = useCallback(() => {
    setSelectedCurrency(undefined)
    setAmount('')
    setHasQuoteError(false)
    setQuotes([])
    setTimeUntilNextQuote(undefined)

    if (abortController) {
      abortController.abort()
      setAbortController(undefined)
    }
  }, [abortController])

  const handleQuoteRefreshInternal = useCallback(
    async (overrides: BuyParamOverrides) => {
      const params = {
        country: overrides.country ?? selectedCountryCode,
        sourceCurrencyCode:
          overrides.sourceCurrencyCode ?? selectedCurrency?.currencyCode,
        destinationCurrencyCode:
          overrides.destinationCurrencyCode ?? selectedMeldAsset?.currencyCode,
        amount: overrides.amount ?? amount,
        receiveAddress: overrides.receiveAddress ?? receiveAddress,
        paymentMethod: overrides.paymentMethod ?? selectedPaymentMethod,
      }

      if (!params.sourceCurrencyCode || !params.destinationCurrencyCode) {
        return
      }

      const amountWrapped = new Amount(params.amount)
      const isAmountEmpty =
        amountWrapped.isZero()
        || amountWrapped.isNaN()
        || amountWrapped.isUndefined()

      if (isAmountEmpty) {
        return
      }

      const controller = new AbortController()
      setAbortController(controller)
      setIsFetchingQuotes(true)
      setHasQuoteError(false)

      let quoteResponse
      try {
        quoteResponse = await generateQuotes({
          account: params.receiveAddress,
          amount: amountWrapped.toNumber(),
          country: params.country || 'US',
          sourceCurrencyCode: params.sourceCurrencyCode,
          destinationCurrencyCode: params.destinationCurrencyCode,
          paymentMethod: params.paymentMethod,
        }).unwrap()
      } catch (error) {
        console.error('generateQuotes failed', error)
        setIsFetchingQuotes(false)
      }

      if (controller.signal.aborted) {
        setIsFetchingQuotes(false)
        setAbortController(undefined)
        return
      }

      if (quoteResponse?.error) {
        console.error('quoteResponse.error', quoteResponse.error)
        setHasQuoteError(true)
      }

      if (quoteResponse?.cryptoQuotes) {
        setQuotes(quoteResponse.cryptoQuotes)
      }

      setIsFetchingQuotes(false)
      setAbortController(undefined)
      setTimeUntilNextQuote(30000)
    },
    [
      amount,
      selectedCountryCode,
      generateQuotes,
      selectedMeldAsset?.currencyCode,
      selectedCurrency?.currencyCode,
      selectedPaymentMethod,
      receiveAddress,
    ],
  )

  const handleQuoteRefresh = useDebouncedCallback(
    async (overrides: BuyParamOverrides) => {
      await handleQuoteRefreshInternal(overrides)
    },
    700,
  )

  const onSetAmount = useCallback(
    async (value: string) => {
      setAmount(value)
      if (!value) {
        setAmount('')
      }

      setQuotes([])

      await handleQuoteRefresh({
        amount: value,
      })
    },
    [handleQuoteRefresh],
  )

  const onSelectCountry = useCallback(
    async (countryCode: string) => {
      setSelectedCountryCode(countryCode)
      setQuotes([])

      await handleQuoteRefresh({
        country: countryCode,
      })
    },
    [handleQuoteRefresh],
  )

  const onSelectPaymentMethod = useCallback(
    async (paymentMethod: string) => {
      const foundMethod =
        paymentMethods?.find((method) => method.paymentMethod === paymentMethod)
        ?? DEFAULT_PAYMENT_METHOD
      setSelectedPaymentMethod(foundMethod)
      setQuotes([])

      await handleQuoteRefresh({
        paymentMethod: foundMethod,
      })
    },
    [handleQuoteRefresh, paymentMethods],
  )

  const onSelectCurrency = useCallback(
    async (currency: MeldFiatCurrency) => {
      setSelectedCurrency(currency)

      await handleQuoteRefresh({
        sourceCurrencyCode: currency.currencyCode,
      })
    },
    [handleQuoteRefresh],
  )

  const onSelectToken = useCallback(
    async (asset: MeldCryptoCurrency, account?: BraveWallet.AccountInfo) => {
      const incomingAssetsCoinType = getMeldTokensCoinType(asset)
      const accountToUse =
        account !== undefined
          ? account
          : selectedAccount.accountId.coin !== incomingAssetsCoinType
            ? getFirstAccountByCoinType(incomingAssetsCoinType, accounts)
            : selectedAccount
      if (!accountToUse) {
        setPendingSelectedToken(asset)
        setShowCreateAccount(true)
        return
      }
      setShowCreateAccount(false)
      setPendingSelectedToken(undefined)
      history.replace(makeFundWalletRoute(asset, accountToUse))
      setQuotes([])

      await handleQuoteRefresh({
        destinationCurrencyCode: asset?.currencyCode,
      })
    },
    [handleQuoteRefresh, history, selectedAccount, accounts],
  )

  const onSelectAccount = useCallback(
    (account: BraveWallet.AccountInfo) => {
      history.replace(makeFundWalletRoute(selectedMeldAsset, account))
    },
    [selectedMeldAsset, history],
  )

  const onBuy = useCallback(
    async (quote: MeldCryptoQuote) => {
      if (!quote.serviceProvider || !selectedCurrency) return

      const sessionData: CryptoBuySessionData = {
        countryCode: selectedCountryCode,
        destinationCurrencyCode: selectedMeldAsset.currencyCode,
        paymentMethodType: selectedPaymentMethod.paymentMethod,
        redirectUrl: undefined,
        serviceProvider: quote.serviceProvider,
        sourceAmount: amount,
        sourceCurrencyCode: selectedCurrency.currencyCode,
        walletAddress: receiveAddress,
        walletTag: undefined,
        lockFields: ['walletAddress'],
      }

      const customerData: CryptoWidgetCustomerData = {
        customer: undefined,
        customerId: undefined,
        externalCustomerId: undefined,
        externalSessionId: undefined,
      }

      try {
        setIsCreatingWidgetFor(quote.serviceProvider)
        const { widget } = await createMeldBuyWidget({
          sessionData,
          customerData,
        }).unwrap()
        setIsCreatingWidgetFor(undefined)

        if (!widget) {
          return
        }

        const { widgetUrl } = widget
        if (chrome.tabs !== undefined) {
          chrome.tabs.create({ url: widgetUrl }, () => {
            if (chrome.runtime.lastError) {
              console.error(
                'tabs.create failed: ' + chrome.runtime.lastError.message,
              )
            }
          })
        } else {
          // Tabs.create is desktop specific. Using window.open for android.
          window.open(widgetUrl, '_blank', 'noopener noreferrer')
        }
      } catch (error) {
        console.error('createMeldBuyWidget failed', error)
        setIsCreatingWidgetFor(undefined)
      }
    },
    [
      amount,
      createMeldBuyWidget,
      selectedMeldAsset?.currencyCode,
      selectedCountryCode,
      selectedCurrency,
      receiveAddress,
      selectedPaymentMethod,
    ],
  )

  const onCloseCreateAccount = useCallback(() => {
    if (!pendingSelectedToken) {
      history.push(WalletRoutes.FundWalletPageStart)
    }
    setShowCreateAccount(false)
  }, [pendingSelectedToken, history])

  // Effects
  useEffect(() => {
    if (fiatCurrencies && fiatCurrencies.length > 0 && !selectedCurrency) {
      const defaultCurrency = fiatCurrencies.find(
        (currency) =>
          currency.currencyCode.toLowerCase()
          === defaultFiatCurrency.toLowerCase(),
      )
      setSelectedCurrency(defaultCurrency)
    }
  }, [defaultFiatCurrency, fiatCurrencies, selectedCurrency])

  // Fetch quotes in intervals
  useEffect(() => {
    const interval = setInterval(async () => {
      if (timeUntilNextQuote && timeUntilNextQuote !== 0) {
        setTimeUntilNextQuote(timeUntilNextQuote - 1000)
        return
      }
      if (!isFetchingQuotes) {
        await handleQuoteRefresh({})
      }
    }, 1000)
    return () => {
      clearInterval(interval)
    }
  }, [handleQuoteRefresh, timeUntilNextQuote, isFetchingQuotes])

  useEffect(() => {
    // Reset quotes and triggers a new fetch if state is reset
    // back to defaults on the page.
    if (
      selectedMeldAsset.currencyCode === DEFAULT_ASSET.currencyCode
      && ((hasQuoteError && quotes.length === 0)
        || (quotes.length !== 0
          && quotes[0].destinationCurrencyCode
            !== selectedMeldAsset.currencyCode))
      && currencyCode === undefined
      && chainId === undefined
    ) {
      setQuotes([])
      setHasQuoteError(false)
      setTimeUntilNextQuote(undefined)
    }
  }, [selectedMeldAsset, currencyCode, chainId, quotes, hasQuoteError])

  useEffect(() => {
    if (
      selectedMeldAsset.currencyCode !== DEFAULT_ASSET.currencyCode
      && currencyCode !== undefined
      && chainId !== undefined
      && selectedAccount === undefined
    ) {
      setShowCreateAccount(true)
    }
  }, [selectedMeldAsset, currencyCode, chainId, selectedAccount])

  return {
    selectedMeldAsset,
    selectedCurrency,
    selectedAccount,
    amount,
    isLoadingAssets,
    formattedCryptoEstimate,
    fiatCurrencies,
    cryptoCurrencies: meldSupportedBuyAssets,
    countries,
    accounts,
    defaultFiatCurrency,
    isFetchingQuotes,
    quotes,
    filteredQuotes,
    onSelectToken,
    onSelectAccount,
    onSelectCurrency,
    onSetAmount,
    serviceProviders,
    selectedCountryCode,
    isLoadingPaymentMethods,
    isLoadingCountries,
    paymentMethods,
    selectedPaymentMethod,
    onSelectCountry,
    onSelectPaymentMethod,
    onBuy,
    isCreatingWidgetFor,
    searchTerm,
    onSearch: setSearchTerm,
    cryptoEstimate,
    hasQuoteError,
    isLoadingServiceProvider,
    reset,
    showCreateAccount,
    onCloseCreateAccount,
    pendingSelectedToken,
  }
}
