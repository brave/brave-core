// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { useSelector } from 'react-redux'
import { BraveWallet, WalletState } from '../../constants/types'

// Hooks
import useBalance from './balance'

// Utils
import Amount from '../../utils/amount'

export default function usePreset (
  {
    asset,
    onSetAmount
  }: {
    onSetAmount?: (value: string) => void
    asset?: BraveWallet.BlockchainToken
  }
) {
  // redux
  const { selectedAccount, networkList } = useSelector((state: { wallet: WalletState }) => state.wallet)

  const getBalance = useBalance(networkList)

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
