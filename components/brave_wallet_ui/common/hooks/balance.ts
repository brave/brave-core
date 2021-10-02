// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { AccountAssetOptionType, WalletAccountType } from '../../constants/types'
import { formatBalance } from '../../utils/format-balances'

export default function useBalance (selectedAccount: WalletAccountType) {
  return React.useCallback((asset: AccountAssetOptionType) => {
    let assetBalance = '0'
    let fiatBalance = '0'

    if (!selectedAccount || !selectedAccount.tokens) {
      return { assetBalance, fiatBalance }
    }

    const token = selectedAccount.tokens.find(
      (token) => token.asset.symbol === asset.asset.symbol
    )
    if (!token) {
      return { assetBalance, fiatBalance }
    }

    return {
      assetBalance: formatBalance(token.assetBalance, token.asset.decimals),
      fiatBalance: token.fiatBalance
    }
  }, [selectedAccount])
}
