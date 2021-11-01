// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import BigNumber from 'bignumber.js'
import { formatBalance } from '../../utils/format-balances'
import { WalletAccountType, AccountAssetOptionType } from '../../constants/types'

export default function usePreset (
  selectedAccount: WalletAccountType,
  swapAsset: AccountAssetOptionType,
  sendAsset: AccountAssetOptionType,
  onSetFromAmount: (value: string) => void,
  onSetSendAmount: (value: string) => void
) {
  return (sendOrSwap: 'send' | 'swap') => (percent: number) => {
    const selectedAsset = sendOrSwap === 'send' ? sendAsset : swapAsset
    const asset = selectedAccount.tokens.find(
      (token) => (
        token.asset.contractAddress === selectedAsset.asset.contractAddress ||
        token.asset.symbol === selectedAsset.asset.symbol
      )
    )
    const amount = new BigNumber(asset?.assetBalance || '0').times(percent).toString()
    const formattedAmount = formatBalance(amount.toString(), asset?.asset.decimals ?? 18)

    if (sendOrSwap === 'send') {
      onSetSendAmount(formattedAmount)
    } else {
      onSetFromAmount(formattedAmount)
    }
  }
}
