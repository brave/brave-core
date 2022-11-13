// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet, P3ASendTransactionTypes, SupportedTestNetworks } from '../constants/types'
import { getLocale } from '../../common/locale'
import { SolanaTransactionTypes } from '../common/constants/solana'
import { loadTimeData } from '../../common/loadTimeData'

type Order = 'ascending' | 'descending'

export const sortTransactionByDate = (transactions: BraveWallet.TransactionInfo[], order: Order = 'ascending') => {
  return [...transactions].sort(function (x: BraveWallet.TransactionInfo, y: BraveWallet.TransactionInfo) {
    return order === 'ascending'
      ? Number(x.createdTime.microseconds) - Number(y.createdTime.microseconds)
      : Number(y.createdTime.microseconds) - Number(x.createdTime.microseconds)
  })
}

export const getTransactionStatusString = (statusId: number) => {
  switch (statusId) {
    case BraveWallet.TransactionStatus.Unapproved:
      return getLocale('braveWalletTransactionStatusUnapproved')
    case BraveWallet.TransactionStatus.Approved:
      return getLocale('braveWalletTransactionStatusApproved')
    case BraveWallet.TransactionStatus.Rejected:
      return getLocale('braveWalletTransactionStatusRejected')
    case BraveWallet.TransactionStatus.Submitted:
      return getLocale('braveWalletTransactionStatusSubmitted')
    case BraveWallet.TransactionStatus.Confirmed:
      return getLocale('braveWalletTransactionStatusConfirmed')
    case BraveWallet.TransactionStatus.Error:
      return getLocale('braveWalletTransactionStatusError')
    case BraveWallet.TransactionStatus.Dropped:
      return getLocale('braveWalletTransactionStatusDropped')
    default:
      return ''
  }
}

export function isSolanaTransaction (transaction: BraveWallet.TransactionInfo) {
  const { txType, txDataUnion: { solanaTxData } } = transaction
  return SolanaTransactionTypes.includes(txType) ||
    (txType === BraveWallet.TransactionType.Other && solanaTxData !== undefined)
}

export function shouldReportTransactionP3A (txInfo: BraveWallet.TransactionInfo, network: BraveWallet.NetworkInfo, coin: BraveWallet.CoinType) {
  if (P3ASendTransactionTypes.includes(txInfo.txType) ||
    (coin === BraveWallet.CoinType.FIL && txInfo.txType === BraveWallet.TransactionType.Other)) {
    const countTestNetworks = loadTimeData.getBoolean(BraveWallet.P3A_COUNT_TEST_NETWORKS_LOAD_TIME_KEY)
    return countTestNetworks || !SupportedTestNetworks.includes(network.chainId)
  }
  return false
}
