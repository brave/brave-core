// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AccountPageTabs, BraveWallet, WalletRoutes } from '../constants/types'

export const makeAccountRoute = (
  accountInfo: BraveWallet.AccountInfo,
  tab: AccountPageTabs
) => {
  const id = accountInfo.address || accountInfo.accountId.uniqueKey
  return WalletRoutes.Account.replace(':accountId', id).replace(
    ':selectedTab?',
    tab
  )
}

export const makeAccountTransactionRoute = (
  accountInfo: BraveWallet.AccountInfo,
  transactionIdHash: string
) => {
  const id = accountInfo.address || accountInfo.accountId.uniqueKey
  return (
    WalletRoutes.Account.replace(':accountId', id).replace(
      ':selectedTab?',
      AccountPageTabs.AccountTransactionsSub
    ) + transactionIdHash
  )
}
