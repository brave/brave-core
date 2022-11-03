// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  P3ASendTransactionTypes,
  SupportedTestNetworks
} from '../constants/types'
import { SolanaTransactionTypes } from '../common/constants/solana'

// utils
import { getLocale } from '../../common/locale'
import { loadTimeData } from '../../common/loadTimeData'
import { getTypedSolanaTxInstructions } from './solana-instruction-utils'

type Order = 'ascending' | 'descending'

type FileCoinTransactionInfo = BraveWallet.TransactionInfo & {
  txDataUnion: {
    filTxData: BraveWallet.FilTxData
  }
}

export type SolanaTransactionInfo = BraveWallet.TransactionInfo & {
  txDataUnion: {
    solanaTxData: BraveWallet.SolanaTxData
  }
}

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

export const getTransactionNonce = (tx: BraveWallet.TransactionInfo): string => {
  return tx.txDataUnion?.ethTxData1559?.baseData.nonce || ''
}

export function isSolanaDappTransaction (tx: BraveWallet.TransactionInfo): tx is SolanaTransactionInfo {
  return (
    tx.txDataUnion.solanaTxData !== undefined &&
    [
      BraveWallet.TransactionType.SolanaDappSignTransaction,
      BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
      BraveWallet.TransactionType.SolanaSwap,
      BraveWallet.TransactionType.Other
    ].includes(tx.txType)
  )
}

export function isFilecoinTransaction (tx: BraveWallet.TransactionInfo): tx is FileCoinTransactionInfo {
  return tx.txDataUnion.filTxData !== undefined
}

export const getToAddressesFromSolanaTransaction = (
  tx: SolanaTransactionInfo
) => {
  const { solanaTxData } = tx.txDataUnion
  const instructions = getTypedSolanaTxInstructions(solanaTxData)
  const to = solanaTxData?.toWalletAddress ?? ''

  if (to) {
    return [to]
  }

  const addresses = instructions.map((instruction) => {
    switch (instruction.type) {
      case 'Transfer':
      case 'TransferWithSeed':
      case 'WithdrawNonceAccount': {
        const { toPubkey } = instruction.params
        return toPubkey.toString() ?? ''
      }

      case 'Create':
      case 'CreateWithSeed': {
        const { newAccountPubkey } = instruction.params
        return newAccountPubkey.toString() ?? ''
      }

      case 'Unknown': {
        return solanaTxData?.instructions[0]?.accountMetas[0]?.pubkey.toString() ?? ''
      }

      default: return to ?? ''
    }
  })

  return [...new Set(addresses.filter(a => !!a))] // unique, non empty addresses
}
