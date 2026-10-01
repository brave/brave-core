// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import {
  MeldCountry,
  MeldCryptoCurrency,
  MeldFiatCurrency,
  MeldCryptoQuote,
  MeldServiceProvider,
  MeldPaymentMethod,
  CryptoWidgetCustomerData,
  CryptoBuySessionData,
  MeldCryptoWidget,
} from '../../../constants/types'

// Utils
import { handleEndpointError } from '../../../utils/api-utils'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

type GetCryptoQuotesArgs = {
  country: string
  sourceCurrencyCode: string
  destinationCurrencyCode: string
  amount: number
  account: string | null
  paymentMethod: MeldPaymentMethod
}

type GetPaymentMethodsArg = {
  country: string
  sourceCurrencyCode: string
}

type CreateMeldBuyWidgetArgs = {
  sessionData: CryptoBuySessionData
  customerData: CryptoWidgetCustomerData
}

export const meldIntegrationEndpoints = ({
  query,
  mutation,
}: WalletApiEndpointBuilderParams) => {
  return {
    getMeldFiatCurrencies: query<MeldFiatCurrency[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          const { fiatCurrencies, error } =
            await meldIntegrationService.getFiatCurrencies()

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting fiat currencies: ',
              error,
            )
          }

          return {
            data: fiatCurrencies || [],
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting fiat currencies: ',
            error,
          )
        }
      },
      providesTags: ['MeldFiatCurrencies'],
    }),
    getMeldCryptoCurrencies: query<MeldCryptoCurrency[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          const { fiatCurrencies: cryptoCurrencies, error } =
            await meldIntegrationService.getCryptoCurrencies()

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting crypto currencies: ',
              error,
            )
          }

          return {
            data: cryptoCurrencies || [],
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting crypto currencies: ',
            error,
          )
        }
      },
      providesTags: ['MeldCryptoCurrencies'],
    }),
    getMeldCountries: query<MeldCountry[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          const { countries, error } =
            await meldIntegrationService.getCountries()
          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting countries: ',
              error,
            )
          }

          return {
            data: countries || [],
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting countries: ',
            error,
          )
        }
      },
      providesTags: ['MeldCountries'],
    }),
    getMeldServiceProviders: query<MeldServiceProvider[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data

          const { serviceProviders, error } =
            await meldIntegrationService.getServiceProviders()

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting countries: ',
              error,
            )
          }

          return {
            data: serviceProviders || [],
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting service providers: ',
            error,
          )
        }
      },
      providesTags: ['MeldServiceProviders'],
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
            destinationCurrencyCode,
            amount,
            account,
            paymentMethod,
          } = params

          const result = await meldIntegrationService.getCryptoQuotes(
            country,
            sourceCurrencyCode,
            destinationCurrencyCode,
            amount,
            account,
            paymentMethod.paymentMethod,
          )

          return {
            data: result,
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting fetching quotes: ',
            error,
          )
        }
      },
      invalidatesTags: ['MeldCryptoQuotes'],
    }),
    getMeldPaymentMethods: query<MeldPaymentMethod[], GetPaymentMethodsArg>({
      queryFn: async (params, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { meldIntegrationService } = baseQuery(undefined).data
          const { paymentMethods, error } =
            await meldIntegrationService.getPaymentMethods(
              params.country,
              params.sourceCurrencyCode,
            )

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error getting paymentMethods: ',
              error,
            )
          }

          return {
            data: paymentMethods || [],
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting paymentMethods: ',
            error,
          )
        }
      },
      providesTags: ['MeldPaymentMethods'],
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
              customerData,
            )

          if (error) {
            return handleEndpointError(
              endpoint,
              'Error in createMeldBuyWidget: ',
              error,
            )
          }

          return {
            data: {
              widget: widgetData,
            },
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error in createMeldBuyWidget: ',
            error,
          )
        }
      },
      invalidatesTags: ['MeldWidget'],
    }),
  }
}
