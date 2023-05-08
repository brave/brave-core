// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// hooks
import {
  useGetUserTokensRegistryQuery,
  useGetTokensRegistryQuery
} from '../slices/api.slice'

// utils
import {
  selectAllUserAssetsFromQueryResult,
  selectAllBlockchainTokensFromQueryResult,
  selectCombinedTokensList
} from '../slices/entities/blockchain-token.entity'

export const useGetCombinedTokensListQuery = (
  arg?: undefined,
  opts?: { skip?: boolean }
) => {
  const { isLoadingUserTokens, userTokens } = useGetUserTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        isLoadingUserTokens: res.isLoading,
        userTokens: selectAllUserAssetsFromQueryResult(res)
      }),
      skip: opts?.skip
    }
  )

  const { isLoadingKnownTokens, knownTokens } = useGetTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        isLoadingKnownTokens: res.isLoading,
        knownTokens: selectAllBlockchainTokensFromQueryResult(res)
      }),
      skip: opts?.skip
    }
  )

  const combinedQuery = React.useMemo(() => {
    if (isLoadingUserTokens || isLoadingKnownTokens) {
      return {
        isLoading: true,
        data: []
      }
    }
    const combinedList = selectCombinedTokensList(knownTokens, userTokens)
    return {
      isLoading: isLoadingUserTokens || isLoadingKnownTokens,
      data: combinedList
    }
  }, [isLoadingKnownTokens, isLoadingUserTokens, userTokens, knownTokens])

  return combinedQuery
}
