// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'

// Utils
import { getLocale } from '../../common/locale'
import { getTransactionStatusString } from './tx-utils'

export const errorTxTypes = [
  BraveWallet.TransactionStatus.Error,
  BraveWallet.TransactionStatus.Dropped,
  BraveWallet.TransactionStatus.Rejected,
]

export function getGate3EffectiveStatus(
  swapStatusCode: BraveWallet.Gate3SwapStatusCode,
): { status: BraveWallet.TransactionStatus; label: string } | undefined {
  switch (swapStatusCode) {
    case BraveWallet.Gate3SwapStatusCode.kPending:
      return {
        status: BraveWallet.TransactionStatus.Submitted,
        label: getLocale('braveWalletSwapPending'),
      }
    case BraveWallet.Gate3SwapStatusCode.kProcessing:
      return {
        status: BraveWallet.TransactionStatus.Submitted,
        label: getLocale('braveWalletSwapProcessing'),
      }
    case BraveWallet.Gate3SwapStatusCode.kSuccess:
      return {
        status: BraveWallet.TransactionStatus.Confirmed,
        label: getTransactionStatusString(
          BraveWallet.TransactionStatus.Confirmed,
        ),
      }
    case BraveWallet.Gate3SwapStatusCode.kFailed:
      return {
        status: BraveWallet.TransactionStatus.Error,
        label: getTransactionStatusString(BraveWallet.TransactionStatus.Error),
      }
    case BraveWallet.Gate3SwapStatusCode.kRefunded:
      return {
        status: BraveWallet.TransactionStatus.Error,
        label: getLocale('braveWalletSwapRefunded'),
      }
    default:
      return undefined
  }
}
