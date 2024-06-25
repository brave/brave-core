// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
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
  walletApi
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

  // state
  const [selectedAsset, setSelectedAsset] = React.useState<
    MeldCryptoCurrency | undefined
  >(undefined)
  const [selectedCurrency, setSelectedCurrency] = React.useState<
    MeldFiatCurrency | undefined
  >(undefined)
  const [selectedAccount, setSelectedAccount] =
    React.useState<BraveWallet.AccountInfo>(accounts[0])
  const [amount, setAmount] = React.useState<string>('')
  const [abortController, setAbortController] = React.useState<
    AbortController | undefined
  >(undefined)
  const [isFetchingQuotes, setIsFetchingQuotes] = React.useState(false)
  const [buyErrors, setBuyErrors] = React.useState<string[] | undefined>([])
  const [quotes, setQuotes] = React.useState<MeldCryptoQuote[] | undefined>([])
  const [timeUnitNextQuote, setTimeUnitNextQuote] = React.useState<
    number | undefined
  >(undefined)

  // computed
  const tokenPriceIds: string[] = []
  // React.useMemo(() => {
  //   return cryptoCurrencies?.map((asset) => getAssetPriceId(asset)) ?? []
  // }, [cryptoCurrencies])

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
  const selectedAssetSpotPrice = React.useMemo(() => {
    if (selectedAsset && spotPriceRegistry) {
      return spotPriceRegistry[getAssetPriceId(selectedAsset)]
    }
    return undefined
  }, [selectedAsset, spotPriceRegistry])

  const estimatedCryptoAmount = React.useMemo(() => {
    if (selectedAssetSpotPrice && selectedAsset) {
      const symbol = getAssetSymbol(selectedAsset)
      return new Amount(amount)
        .div(selectedAssetSpotPrice.price)
        .formatAsAsset(5, symbol)
    }
    return ''
  }, [selectedAssetSpotPrice, selectedAsset, amount])

  // methods
  const reset = React.useCallback(() => {
    setSelectedAsset(undefined)
    setSelectedCurrency(undefined)
    setSelectedAccount(accounts[0])
    setAmount('')
    setBuyErrors([])
    setQuotes([])
    setTimeUnitNextQuote(undefined)

    if (abortController) {
      abortController.abort()
      setAbortController(undefined)
    }
  }, [abortController, accounts])

  const handleQuoteRefreshInternal = React.useCallback(
    async (overrides: BuyParamOverrides) => {
      const assetSymbol = selectedAsset
        ? getAssetSymbol(selectedAsset)
        : undefined
      const params = {
        country: overrides.country || defaultCountryCode || 'US',
        sourceCurrencyCode:
          overrides.sourceCurrencyCode || selectedCurrency?.currencyCode,
        destionationCurrencyCode:
          overrides.destionationCurrencyCode || assetSymbol,
        amount: overrides.amount || new Amount(amount).toNumber(),
        account: overrides.account || selectedAccount.address
      }

      if (
        !params.sourceCurrencyCode ||
        !params.destionationCurrencyCode ||
        !params.account
      ) {
        return
      }

      const amountWrapped = new Amount(amount)
      const isAmountEmpty =
        amountWrapped.isZero() ||
        amountWrapped.isNaN() ||
        amountWrapped.isUndefined()

      if (isAmountEmpty) {
        reset()
        return
      }

      const controller = new AbortController()
      setAbortController(controller)
      setIsFetchingQuotes(true)

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
        setBuyErrors(quoteResponse.error)
      }

      if (quoteResponse?.cryptoQuotes) {
        setQuotes(quoteResponse.cryptoQuotes)
      }

      setIsFetchingQuotes(false)
      setAbortController(undefined)
      setTimeUnitNextQuote(10000)
    },
    [
      amount,
      defaultCountryCode,
      defaultFiatCurrency,
      generateQuotes,
      reset,
      selectedAccount?.address,
      selectedAsset?.currencyCode,
      selectedCurrency?.currencyCode
    ]
  )

  const handleQuoteRefresh = useDebouncedCallback(
    async (overrides: BuyParamOverrides) => {
      console.log('fetching quotes', overrides)
      await handleQuoteRefreshInternal(overrides)
    },
    700
  )

  const onSetAmount = React.useCallback(
    async (value: string) => {
      setAmount(value)
      if (!value) {
        setAmount('')
      }

      await handleQuoteRefresh({
        amount: value,
      })
    },
    [handleQuoteRefresh]
  )

  const onSelectCurrency = React.useCallback(
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

  const onSelectAccount = React.useCallback(
    async (account: BraveWallet.AccountInfo) => {
      setSelectedAccount(account)

      await handleQuoteRefresh({
        account: account.address
      })
    },
    [handleQuoteRefresh]
  )

  const onSelectToken = React.useCallback(
    async (asset: MeldCryptoCurrency) => {
      setSelectedAsset(asset)

      await handleQuoteRefresh({
        destionationCurrencyCode: getAssetSymbol(asset)
      })
    },
    [handleQuoteRefresh]
  )

  // effects
  React.useEffect(() => {
    if (accounts.length > 0 && !selectedAccount) {
      setSelectedAccount(accounts[0])
    }
  }, [accounts, selectedAccount])

  React.useEffect(() => {
    if (cryptoCurrencies && cryptoCurrencies.length > 0 && !selectedAsset) {
      setSelectedAsset(cryptoCurrencies[0])
    }
  }, [cryptoCurrencies, selectedAsset])

  React.useEffect(() => {
    if (fiatCurrencies && fiatCurrencies.length > 0 && !selectedCurrency) {
      const defaultCurrency = fiatCurrencies.find(
        (currency) =>
          currency.currencyCode.toLowerCase() ===
          defaultFiatCurrency.toLowerCase()
      )
      setSelectedCurrency(defaultCurrency || fiatCurrencies[0])
    }
  }, [defaultFiatCurrency, fiatCurrencies, selectedCurrency])

  return {
    selectedAsset,
    selectedCurrency,
    selectedAccount,
    amount,
    isLoadingAssets,
    isLoadingSpotPrices,
    estimatedCryptoAmount,
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
    timeUnitNextQuote,
    onSelectToken,
    onSelectAccount,
    onSelectCurrency,
    onSetAmount
  }
}
