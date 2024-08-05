// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// constants
import { LOCAL_STORAGE_KEYS } from '../constants/local-storage-keys'

// hooks
import { useLocalStorage } from './use_local_storage'
import { useAccountsQuery } from '../slices/api.slice.extra'

export const usePortfolioAccounts = () => {
  // local-storage
  const [filteredOutPortfolioAccountIds] = useLocalStorage<string[]>(
    LOCAL_STORAGE_KEYS.FILTERED_OUT_PORTFOLIO_ACCOUNT_IDS,
    []
  )

  // queries
  const { accounts, isLoading: isLoadingAccounts } = useAccountsQuery()

  // memos
  const usersFilteredAccounts = React.useMemo(() => {
    return accounts.filter(
      (account) =>
        !filteredOutPortfolioAccountIds.includes(account.accountId.uniqueKey)
    )
  }, [accounts, filteredOutPortfolioAccountIds])

  // render
  return { usersFilteredAccounts, isLoadingAccounts }
}
