// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AllAccountsOptionUniqueKey = 'all'

export const AllAccountsOption: BraveWallet.AccountInfo = {
  address: AllAccountsOptionUniqueKey,
  accountId: {
    coin: 0,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: AllAccountsOptionUniqueKey,
    bitcoinAccountIndex: 0,
    uniqueKey: AllAccountsOptionUniqueKey
  },
  name: getLocale('braveWalletAccountFilterAllAccounts'),
  hardware: undefined
}

export const isAllAccountsOptionFilter = (selectedAccountFilter: string) => {
  return selectedAccountFilter === AllAccountsOptionUniqueKey
}

export const applySelectedAccountFilter = (
  accounts: BraveWallet.AccountInfo[],
  selectedAccountFilter: string
): {
  accounts: BraveWallet.AccountInfo[]
  allAccounts?: BraveWallet.AccountInfo[]
  oneAccount?: BraveWallet.AccountInfo
} => {
  if (selectedAccountFilter === AllAccountsOptionUniqueKey) {
    return {
      accounts: accounts,
      allAccounts: accounts,
      oneAccount: undefined
    }
  }

  const account = accounts.find(
    (account) => account.accountId.uniqueKey === selectedAccountFilter
  )
  if (account) {
    return {
      accounts: [account],
      allAccounts: undefined,
      oneAccount: account
    }
  }

  return {
    accounts: [],
    allAccounts: undefined,
    oneAccount: undefined
  }
}
