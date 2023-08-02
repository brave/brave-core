// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Constants
import { BraveWallet, CoinType } from '../../constants/types'

// Utils
import { WalletSelectors } from '../selectors'
import { useGetSelectedChainQuery } from '../slices/api.slice'
import { useUnsafeWalletSelector } from './use-safe-selector'

export function useHasAccount () {
  // redux
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // memos
  const hasSolAccount = React.useMemo((): boolean => {
    return accounts.some(
      (account) => account.accountId.coin === CoinType.SOL
    )
  }, [accounts])
  const hasFilAccount = React.useMemo((): boolean => {
    const keyringForCurrentNetwork =
      selectedNetwork?.chainId === BraveWallet.FILECOIN_MAINNET
        ? BraveWallet.KeyringId.kFilecoin
        : BraveWallet.KeyringId.kFilecoinTestnet
    return accounts.some(
      (account) =>
        account.accountId.coin === CoinType.FIL &&
        account.accountId.keyringId === keyringForCurrentNetwork
    )
  }, [accounts, selectedNetwork])

  const needsAccount = React.useMemo((): boolean => {
    if (accounts.length === 0) {
      // still loading accounts
      return false
    }

    switch (selectedNetwork?.coin) {
      case CoinType.SOL: return !hasSolAccount
      case CoinType.FIL: return !hasFilAccount
      default: return false
    }
  }, [hasSolAccount, hasFilAccount, selectedNetwork])

  return {
    hasSolAccount,
    hasFilAccount,
    needsAccount
  }
}

export default useHasAccount
