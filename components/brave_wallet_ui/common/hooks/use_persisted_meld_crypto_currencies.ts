// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useGetMeldCryptoCurrenciesQuery } from '../slices/api.slice'

// Utils
import {
  getPersistedMeldCryptoCurrencies, //
} from '../../utils/local-storage-utils'

type MeldCryptoQueryOptions = Parameters<
  typeof useGetMeldCryptoCurrenciesQuery
>[1]

/**
 * Wraps useGetMeldCryptoCurrenciesQuery with a localStorage fallback so the
 * last-known on-ramp asset list is available immediately while fresh data loads.
 */
export const usePersistedMeldCryptoCurrenciesQuery = (
  options?: MeldCryptoQueryOptions,
) => {
  return useGetMeldCryptoCurrenciesQuery(undefined, {
    ...options,
    selectFromResult: (res) => {
      if (options?.skip) {
        return res
      }
      const persisted = res.data
        ? undefined
        : getPersistedMeldCryptoCurrencies()
      const data = res.data ?? persisted ?? []
      const hasFallbackData = !!persisted?.length
      return {
        ...res,
        data,
        isLoading: res.isLoading && !hasFallbackData,
      }
    },
  })
}
