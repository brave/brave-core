// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { BraveWallet, WalletAccountType } from '../../constants/types'

// Hooks
import useBalance from './balance'

// Utils
import Amount from '../../utils/amount'

export default function usePreset (
  selectedAccount: WalletAccountType,
  selectedNetwork: BraveWallet.NetworkInfo,
  onSetFromAmount: (value: string) => void,
  onSetSendAmount: (value: string) => void,
  swapAsset?: BraveWallet.BlockchainToken,
  sendAsset?: BraveWallet.BlockchainToken
) {
  const getBalance = useBalance(selectedNetwork)

  return (sendOrSwap: 'send' | 'swap') => (percent: number) => {
    const selectedAsset = sendOrSwap === 'send' ? sendAsset : swapAsset
    if (!selectedAsset) {
      return
    }

    const assetBalance = getBalance(selectedAccount, selectedAsset) || '0'
    const amountWrapped = new Amount(assetBalance).times(percent)

    const formattedAmount = (percent === 1)
      ? amountWrapped.divideByDecimals(selectedAsset.decimals).format()
      : amountWrapped.divideByDecimals(selectedAsset.decimals).format(6)

    if (sendOrSwap === 'send') {
      onSetSendAmount(formattedAmount)
    } else {
      onSetFromAmount(formattedAmount)
    }
  }
}
