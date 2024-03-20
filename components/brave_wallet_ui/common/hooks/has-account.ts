// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Constants
import { BraveWallet } from '../../constants/types'

// Hooks
import { useGetSelectedChainQuery } from '../slices/api.slice'
import { useAccountsQuery } from '../slices/api.slice.extra'

export function useHasAccount() {
  // redux
  const { accounts } = useAccountsQuery()

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // memos
  const hasSolAccount = React.useMemo((): boolean => {
    return accounts.some(
      (account) => account.accountId.coin === BraveWallet.CoinType.SOL
    )
  }, [accounts])
  const hasFilAccount = React.useMemo((): boolean => {
    const keyringForCurrentNetwork =
      selectedNetwork?.chainId === BraveWallet.FILECOIN_MAINNET
        ? BraveWallet.KeyringId.kFilecoin
        : BraveWallet.KeyringId.kFilecoinTestnet
    return accounts.some(
      (account) =>
        account.accountId.coin === BraveWallet.CoinType.FIL &&
        account.accountId.keyringId === keyringForCurrentNetwork
    )
  }, [accounts, selectedNetwork])

  const needsAccount = React.useMemo((): boolean => {
    if (accounts.length === 0) {
      // still loading accounts
      return false
    }

    switch (selectedNetwork?.coin) {
      case BraveWallet.CoinType.SOL:
        return !hasSolAccount
      case BraveWallet.CoinType.FIL:
        return !hasFilAccount
      default:
        return false
    }
  }, [accounts.length, selectedNetwork?.coin, hasSolAccount, hasFilAccount])

  return {
    hasSolAccount,
    hasFilAccount,
    needsAccount
  }
}

export default useHasAccount
