// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  MeldCountry,
  MeldCryptoCurrency,
  MeldFiatCurrency,
  MeldFilter,
  MeldCryptoQuote,
  MeldServiceProvider,
  SupportedOnRampNetworks,
  MeldPaymentMethod,
  CryptoWidgetCustomerData,
  CryptoBuySessionData,
  MeldCryptoWidget
} from '../../../constants/types'
import { handleEndpointError } from '../../../utils/api-utils'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

type GetCryptoQuotesArgs = {
  country: string
  sourceCurrencyCode: string
  destionationCurrencyCode: string
  amount: number
  account: string | null
  paymentMethods: MeldPaymentMethod[]
}

type CreateMeldBuyWidgetArgs = {
  sessionData: CryptoBuySessionData
  customerData: CryptoWidgetCustomerData
}

export const meldIntegrationEndpoints = ({
  query,
  mutation
}: WalletApiEndpointBuilderParams) => {
  return {
    getMeldFiatCurrencies: query<MeldFiatCurrency[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          // get all fiat currencies
          const filter: MeldFilter = {
            countries: undefined,
            fiatCurrencies: undefined,
            cryptoCurrencies: undefined,
            serviceProviders: undefined,
            paymentMethodTypes: undefined,
            statuses: undefined,
            cryptoChains: undefined
          }
          const { fiatCurrencies, error } =
            await meldIntegrationService.getFiatCurrencies(filter)

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting fiat currencies: ',
              error
            )
          }

          return {
            data: fiatCurrencies || []
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting fiat currencies: ',
            error
          )
        }
      },
      providesTags: ['MeldFiatCurrencies']
    }),
    getMeldCryptoCurrencies: query<MeldCryptoCurrency[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          // get all crypto currencies
          const filter: MeldFilter = {
            countries: undefined,
            fiatCurrencies: undefined,
            cryptoCurrencies: undefined,
            serviceProviders: undefined,
            paymentMethodTypes: undefined,
            statuses: undefined,
            cryptoChains: undefined
          }
          const { fiatCurrencies: cryptoCurrencies, error } =
            await meldIntegrationService.getCryptoCurrencies(filter)

          const supportedAssets = cryptoCurrencies?.filter((asset) =>
            SupportedOnRampNetworks.includes(
              '0x' + parseInt(asset?.chainId ?? '').toString(16)
            )
          )

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting crypto currencies: ',
              error
            )
          }

          return {
            data: supportedAssets || []
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting crypto currencies: ',
            error
          )
        }
      },
      providesTags: ['MeldCryptoCurrencies']
    }),
    getDefaultCountry: query<string, void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const defaultCountry: string = await new Promise((resolve) => {
            // TODO(william): implement this for wallet
            // to avoid using braveRewards api
            chrome.braveRewards.getDefaultCountry((defaultCountry) => {
              resolve(defaultCountry)
            })
          })

          return {
            data: defaultCountry
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting default country: ',
            error
          )
        }
      },
      providesTags: ['DefaultCountryCode']
    }),
    getMeldCountries: query<MeldCountry[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          // get all countries
          const filter: MeldFilter = {
            countries: undefined,
            fiatCurrencies: undefined,
            cryptoCurrencies: undefined,
            serviceProviders: undefined,
            paymentMethodTypes: undefined,
            statuses: undefined,
            cryptoChains: undefined
          }
          const { countries, error } =
            await meldIntegrationService.getCountries(filter)
          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting countries: ',
              error
            )
          }

          return {
            data: countries || []
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting countries: ',
            error
          )
        }
      },
      providesTags: ['MeldCountries']
    }),
    getMeldServiceProviders: query<MeldServiceProvider[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          // get all countries
          const filter: MeldFilter = {
            countries: undefined,
            fiatCurrencies: undefined,
            cryptoCurrencies: undefined,
            serviceProviders: undefined,
            paymentMethodTypes: undefined,
            statuses: undefined,
            cryptoChains: undefined
          }
          const { serviceProviders, error } =
            await meldIntegrationService.getServiceProviders(filter)

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting countries: ',
              error
            )
          }

          return {
            data: serviceProviders || []
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting service providers: ',
            error
          )
        }
      },
      providesTags: ['MeldServiceProviders']
    }),
    generateMeldCryptoQuotes: mutation<
      {
        cryptoQuotes: MeldCryptoQuote[] | null
        error: string[] | null
      },
      GetCryptoQuotesArgs
    >({
      queryFn: async (params, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          const {
            country,
            sourceCurrencyCode,
            destionationCurrencyCode,
            amount,
            account
          } = params

          const result = await meldIntegrationService.getCryptoQuotes(
            country,
            sourceCurrencyCode,
            destionationCurrencyCode,
            amount,
            account
          )

          return {
            data: result
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting fetching quotes: ',
            error
          )
        }
      },
      invalidatesTags: ['MeldCryptoQuotes']
    }),
    getMeldPaymentMethods: query<MeldPaymentMethod[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          const filter: MeldFilter = {
            countries: undefined,
            fiatCurrencies: undefined,
            cryptoCurrencies: undefined,
            serviceProviders: undefined,
            paymentMethodTypes: undefined,
            statuses: undefined,
            cryptoChains: undefined
          }
          const { paymentMethods, error } =
            await meldIntegrationService.getPaymentMethods(filter)

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting paymentMethods: ',
              error
            )
          }

          return {
            data: paymentMethods || []
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting paymentMethods: ',
            error
          )
        }
      },
      providesTags: ['MeldPaymentMethods']
    }),
    createMeldBuyWidget: mutation<
      {
        widget: MeldCryptoWidget | null
      },
      CreateMeldBuyWidgetArgs
    >({
      queryFn: async (params, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data
          const { sessionData, customerData } = params
          const { widgetData, error } =
            await meldIntegrationService.cryptoBuyWidgetCreate(
              sessionData,
              customerData
            )

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error in createMeldBuyWidget: ',
              error
            )
          }

          return {
            data: {
              widget: widgetData
            }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error in createMeldBuyWidget: ',
            error
          )
        }
      },
      invalidatesTags: ['MeldWidget']
    })
  }
}
