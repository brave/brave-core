// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'

export const fiatCurrencyEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getDefaultFiatCurrency: query<string, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { braveWalletService } = baseQuery(undefined).data
          const { currency } =
            await braveWalletService.getDefaultBaseCurrency()
          const defaultFiatCurrency = currency.toLowerCase()
          return {
            data: defaultFiatCurrency
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to fetch default fiat currency',
            error
          )
        }
      },
      providesTags: ['DefaultFiatCurrency']
    }),
    setDefaultFiatCurrency: mutation<string, string>({
      queryFn: async (
        currencyArg,
        { endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const { braveWalletService } = baseQuery(undefined).data
          braveWalletService.setDefaultBaseCurrency(currencyArg)
          return {
            data: currencyArg
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to set default fiat currency to ${currencyArg}`,
            error
          )
        }
      },
      invalidatesTags: ['DefaultFiatCurrency']
    }),
  }
}
