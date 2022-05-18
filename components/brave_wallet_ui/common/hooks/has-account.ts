// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import {
  BraveWallet,
  WalletState
} from '../../constants/types'

export function useHasAccount () {
  // redux
  const {
    accounts,
    selectedNetwork
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  const hasSolAccount = React.useMemo((): boolean => { return accounts.some(account => account.coin === BraveWallet.CoinType.SOL) }, [accounts])
  const hasFilAccount = React.useMemo((): boolean => { return accounts.some(account => account.coin === BraveWallet.CoinType.FIL) }, [accounts])

  const needsAccount = React.useMemo((): boolean => {
    if (selectedNetwork.coin === BraveWallet.CoinType.SOL) {
      return !hasSolAccount
    }
    if (selectedNetwork.coin === BraveWallet.CoinType.FIL) {
      return !hasFilAccount
    }
    return false
  }, [hasSolAccount, hasFilAccount, selectedNetwork])
  return {
    hasSolAccount,
    hasFilAccount,
    needsAccount
  }
}

export default useHasAccount
