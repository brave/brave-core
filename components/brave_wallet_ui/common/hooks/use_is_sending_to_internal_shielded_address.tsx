// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet } from '../../constants/types'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Selectors
import { WalletSelectors } from '../selectors'
import { useSafeWalletSelector } from './use-safe-selector'

// Hooks
import {
  useGetAvailableShieldedAccountQuery, //
} from '../slices/api.slice'
import { useAccountsQuery } from '../slices/api.slice.extra'

export const useIsSendingToInternalShieldedAddress = (
  txType?: BraveWallet.ZCashTxType,
  recipient?: string,
) => {
  // Selectors
  const isZCashShieldedTransactionsEnabled = useSafeWalletSelector(
    WalletSelectors.isZCashShieldedTransactionsEnabled,
  )

  // Queries
  const { accounts } = useAccountsQuery()
  const zcashAccountIds = accounts
    .filter((account) => account.accountId.coin === BraveWallet.CoinType.ZEC)
    .map((account) => account.accountId)

  const { data: availableShieldedAccount } =
    useGetAvailableShieldedAccountQuery(
      isZCashShieldedTransactionsEnabled && zcashAccountIds
        ? zcashAccountIds
        : skipToken,
    )

  return (
    !!recipient
    && !!txType
    && txType === BraveWallet.ZCashTxType.kTransparentToOrchard
    && !!availableShieldedAccount
    && availableShieldedAccount.orchardInternalAddress?.toLowerCase()
      === recipient.toLowerCase()
  )
}
export default useIsSendingToInternalShieldedAddress
