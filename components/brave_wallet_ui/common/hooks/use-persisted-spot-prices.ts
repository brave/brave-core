// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useGetTokenSpotPricesQuery } from '../slices/api.slice'

// Utils
import {
  getPersistedSpotPrices, //
} from '../../utils/local-storage-utils'

type SpotPricesQueryArgs = Parameters<typeof useGetTokenSpotPricesQuery>

/**
 * Wraps useGetTokenSpotPricesQuery with a localStorage fallback so that
 * last-known prices are returned immediately while fresh data loads.
 */
export const usePersistedTokenSpotPricesQuery = (
  arg: SpotPricesQueryArgs[0],
  options?: SpotPricesQueryArgs[1],
) => {
  return useGetTokenSpotPricesQuery(arg, {
    ...options,
    selectFromResult: (res) => {
      // Only read from localStorage when the query has no data yet
      // (skipped, loading, or uninitialized).
      const persisted = res.data ? undefined : getPersistedSpotPrices()
      const data = res.data ?? persisted ?? []
      // Suppress loading state when we have persisted prices to show,
      // so consumers render stale prices instead of skeletons.
      const hasFallbackData = !!persisted?.length
      return {
        data,
        isLoading: res.isLoading && !hasFallbackData,
        isFetching: res.isFetching,
      }
    },
  })
}
