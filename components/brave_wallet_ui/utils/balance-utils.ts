// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet, WalletAccountType } from '../constants/types'

// utils
import { createTokenBalanceRegistryKey } from './account-utils'
import Amount from './amount'

export const getBalance = (account?: WalletAccountType, token?: BraveWallet.BlockchainToken) => {
  if (!account || !token) {
    return ''
  }

  if (!account.tokenBalanceRegistry) {
    return ''
  }

  // Return native asset balance
  if (
    token.contractAddress === '' &&
    !token.isErc20 &&

    // Since all coinTypes share the same chainId for localHost networks,
    // we want to make sure we return the right balance for that token.
    account.coin === token.coin
  ) {
    return (account.nativeBalanceRegistry || {})[token.chainId || ''] || ''
  }

  const registryKey = createTokenBalanceRegistryKey(token)
  return (account.tokenBalanceRegistry || {})[registryKey] || ''
}

export const formatTokenBalanceWithSymbol = (balance: string, decimals: number, symbol: string, decimalPlace?: number) => {
  return `${new Amount(balance)
    .divideByDecimals(decimals)
    .format(decimalPlace ?? 6, true)} ${symbol}`
}
