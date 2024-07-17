// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useState, useEffect, useCallback, useMemo } from 'react'
import { skipToken } from '@reduxjs/toolkit/dist/query'

// types
import {
  MeldCryptoCurrency,
  MeldFiatCurrency,
  BraveWallet,
  MeldCryptoQuote
} from '../../../../constants/types'

// api
import {
  useGetDefaultFiatCurrencyQuery,
  useGetMeldFiatCurrenciesQuery,
  useGetMeldCryptoCurrenciesQuery,
  useGetMeldCountriesQuery,
  useGetDefaultCountryQuery,
  useGenerateMeldCryptoQuotesMutation,
  useGetTokenSpotPricesQuery,
  walletApi,
  useGetMeldServiceProvidersQuery
} from '../../../../common/slices/api.slice'
import { useAccountsQuery } from '../../../../common/slices/api.slice.extra'

// constants
import { querySubscriptionOptions60s } from '../../../../common/slices/constants'

// utils
import Amount from '../../../../utils/amount'
import { getAssetSymbol, getAssetPriceId } from '../../../../utils/meld_utils'
import { useDebouncedCallback } from './useDebouncedCallback'

export type BuyParamOverrides = {
  country?: string
  sourceCurrencyCode?: string
  destionationCurrencyCode?: string
  amount?: string
  account?: string
}

const defaultAsset: MeldCryptoCurrency = {
  'currencyCode': 'ETH',
  'name': 'Ethereum',
  'chainCode': 'ETH',
  'chainName': 'Ethereum',
  'chainId': '1',
  'contractAddress': '0x0000000000000000000000000000000000000000',
  'symbolImageUrl': 'https://images-currency.meld.io/crypto/ETH/symbol.png'
}

export const useBuy = () => {
  // queries
  const { data: defaultFiatCurrency = 'USD' } = useGetDefaultFiatCurrencyQuery()
  const { data: fiatCurrencies } = useGetMeldFiatCurrenciesQuery()
  const { data: cryptoCurrencies, isLoading: isLoadingAssets } =
    useGetMeldCryptoCurrenciesQuery()
  const { accounts } = useAccountsQuery()
  const { data: countries } = useGetMeldCountriesQuery()
  const { data: defaultCountryCode } = useGetDefaultCountryQuery()
  const [generateQuotes] = useGenerateMeldCryptoQuotesMutation()
  const { data: serviceProviders = [], isLoading: isLoadingServiceProvider } =
    useGetMeldServiceProvidersQuery()

  // state
  const [selectedAsset, setSelectedAsset] =
    useState<MeldCryptoCurrency>(defaultAsset)
  const [selectedCurrency, setSelectedCurrency] = useState<
    MeldFiatCurrency | undefined
  >(undefined)
  const [selectedAccount, setSelectedAccount] =
    useState<BraveWallet.AccountInfo>(accounts[0])
  const [amount, setAmount] = useState<string>('')
  const [abortController, setAbortController] = useState<
    AbortController | undefined
  >(undefined)
  const [isFetchingQuotes, setIsFetchingQuotes] = useState(false)
  const [buyErrors, setBuyErrors] = useState<string[] | undefined>([])
  const [quotes, setQuotes] = useState<MeldCryptoQuote[]>([])
  const [timeUntilNextQuote, setTimeUntilNextQuote] = useState<
    number | undefined
  >(undefined)

  // computed
  const tokenPriceIds: string[] = useMemo(() => {
    return cryptoCurrencies?.map((asset) => getAssetPriceId(asset)) ?? []
  }, [cryptoCurrencies])

  const { data: spotPriceRegistry, isLoading: isLoadingSpotPrices } =
    useGetTokenSpotPricesQuery(
      tokenPriceIds.length > 0
        ? {
            ids: tokenPriceIds,
            toCurrency: selectedCurrency?.currencyCode || defaultFiatCurrency
          }
        : skipToken,
      querySubscriptionOptions60s
    )

  // memos
  const selectedAssetSpotPrice = useMemo(() => {
    if (selectedAsset && spotPriceRegistry) {
      return spotPriceRegistry[getAssetPriceId(selectedAsset)]
    }
    return undefined
  }, [selectedAsset, spotPriceRegistry])

  const [cryptoEstimate, formattedCryptoEstimate] = useMemo(() => {
    if (selectedAssetSpotPrice && selectedAsset) {
      const symbol = getAssetSymbol(selectedAsset)
      const estimate = new Amount(amount).div(selectedAssetSpotPrice.price)

      return [
        estimate?.toNumber().toString(),
        estimate?.formatAsAsset(5, symbol)
      ]
    }
    return ['', '']
  }, [selectedAssetSpotPrice, selectedAsset, amount])

  // methods
  const reset = useCallback(() => {
    setSelectedAsset(defaultAsset)
    setSelectedCurrency(undefined)
    setSelectedAccount(accounts[0])
    setAmount('')
    setBuyErrors([])
    setQuotes([])
    setTimeUntilNextQuote(undefined)

    if (abortController) {
      abortController.abort()
      setAbortController(undefined)
    }
  }, [abortController, accounts])

  const handleQuoteRefreshInternal = useCallback(
    async (overrides: BuyParamOverrides) => {
      const params = {
        country:
          overrides.country === undefined
            ? defaultCountryCode
            : overrides.country,
        sourceCurrencyCode:
          overrides.sourceCurrencyCode === undefined
            ? selectedCurrency?.currencyCode
            : overrides.sourceCurrencyCode,
        destionationCurrencyCode:
          overrides.destionationCurrencyCode === undefined
            ? selectedAsset?.currencyCode
            : overrides.destionationCurrencyCode,
        amount: overrides.amount === undefined ? amount : overrides.amount,
        account:
          overrides.account === undefined
            ? selectedAccount.address
            : overrides.account
      }

      if (
        !params.sourceCurrencyCode ||
        !params.destionationCurrencyCode ||
        !params.account
      ) {
        return
      }

      const amountWrapped = new Amount(params.amount)
      const isAmountEmpty =
        amountWrapped.isZero() ||
        amountWrapped.isNaN() ||
        amountWrapped.isUndefined()

      if (isAmountEmpty) {
        return
      }

      const controller = new AbortController()
      setAbortController(controller)

      let quoteResponse
      try {
        setIsFetchingQuotes(true)
        quoteResponse = await generateQuotes({
          account: selectedAccount.address,
          amount: amountWrapped.toNumber(),
          country: defaultCountryCode || 'US',
          sourceCurrencyCode: params.sourceCurrencyCode,
          destionationCurrencyCode: params.destionationCurrencyCode
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
        console.log('quoteResponse.error', quoteResponse.error)
        setBuyErrors(quoteResponse.error)
      }

      if (quoteResponse?.cryptoQuotes) {
        setQuotes(quoteResponse.cryptoQuotes)
      }

      setIsFetchingQuotes(false)
      setAbortController(undefined)
      setTimeUntilNextQuote(10000)
    },
    [
      amount,
      defaultCountryCode,
      generateQuotes,
      reset,
      selectedAccount?.address,
      selectedAsset,
      selectedCurrency?.currencyCode,
      quotes
    ]
  )

  const handleQuoteRefresh = useDebouncedCallback(
    async (overrides: BuyParamOverrides) => {
      await handleQuoteRefreshInternal(overrides)
    },
    700
  )

  const onSetAmount = useCallback(
    async (value: string) => {
      setAmount(value)
      if (!value) {
        setAmount('')
      }

      setQuotes([])

      await handleQuoteRefresh({
        amount: value
      })
    },
    [handleQuoteRefresh]
  )

  const onSelectCurrency = useCallback(
    async (currency: MeldFiatCurrency) => {
      setSelectedCurrency(currency)

      if (selectedAsset) {
        // TODO(william): Fetch only the spot price for the selected asset
        walletApi.util.invalidateTags([
          {
            type: 'TokenSpotPrices',
            id: getAssetPriceId(selectedAsset)
          }
        ])
      }

      await handleQuoteRefresh({
        sourceCurrencyCode: currency.currencyCode
      })
    },
    [handleQuoteRefresh, selectedAsset]
  )

  const onSelectToken = useCallback(
    async (asset: MeldCryptoCurrency) => {
      console.log('setting asset to: ', asset)
      setSelectedAsset(asset)

      await handleQuoteRefresh({
        destionationCurrencyCode: asset?.currencyCode
      })
    },
    [handleQuoteRefresh]
  )

  const onFlipAmounts = useCallback(async () => {
    if (!cryptoEstimate) return

    setAmount(cryptoEstimate)
    await handleQuoteRefresh({
      amount: cryptoEstimate
    })
  }, [cryptoEstimate, handleQuoteRefresh])

  // effects
  useEffect(() => {
    if (accounts.length > 0 && !selectedAccount) {
      setSelectedAccount(accounts[0])
    }
  }, [accounts, selectedAccount])

  useEffect(() => {
    if (fiatCurrencies && fiatCurrencies.length > 0 && !selectedCurrency) {
      const defaultCurrency = fiatCurrencies.find(
        (currency) =>
          currency.currencyCode.toLowerCase() ===
          defaultFiatCurrency.toLowerCase()
      )
      setSelectedCurrency(defaultCurrency)
    }
  }, [defaultFiatCurrency, fiatCurrencies, selectedCurrency])

  // fetch quotes in intervals
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

  return {
    selectedAsset,
    selectedCurrency,
    selectedAccount,
    amount,
    isLoadingAssets,
    isLoadingSpotPrices,
    cryptoEstimate,
    formattedCryptoEstimate,
    selectedAssetSpotPrice,
    spotPriceRegistry,
    fiatCurrencies,
    cryptoCurrencies,
    countries,
    accounts,
    defaultCountryCode,
    defaultFiatCurrency,
    handleQuoteRefreshInternal,
    isFetchingQuotes,
    quotes,
    buyErrors,
    timeUntilNextQuote,
    onSelectToken,
    onSelectAccount: setSelectedAccount,
    onSelectCurrency,
    onSetAmount,
    serviceProviders,
    isLoadingServiceProvider,
    onFlipAmounts
  }
}
