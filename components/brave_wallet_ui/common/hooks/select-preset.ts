// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet, WalletAccountType } from '../../constants/types'

// Utils
import Amount from '../../utils/amount'
import { getBalance } from '../../utils/balance-utils'

import { WalletSelectors } from '../selectors'
import { useUnsafeWalletSelector } from './use-safe-selector'

export default function usePreset (
  {
    asset,
    onSetAmount,
    account
  }: {
    onSetAmount?: (value: string) => void
    asset?: BraveWallet.BlockchainToken
    account?: WalletAccountType
  }
) {
  // redux
  const selectedAccount = account ?? useUnsafeWalletSelector(WalletSelectors.selectedAccount)

  return (percent: number) => {
    if (!asset) {
      return
    }

    const assetBalance = getBalance(selectedAccount, asset) || '0'
    const amountWrapped = new Amount(assetBalance).times(percent)

    const formattedAmount = (percent === 1)
      ? amountWrapped.divideByDecimals(asset.decimals).format()
      : amountWrapped.divideByDecimals(asset.decimals).format(6)

    onSetAmount?.(formattedAmount)
  }
}
