/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// utils
import { getLocale } from '../../common/locale'

// types
import {
  BraveWallet
} from '../constants/types'

export function isValidFilAddress (value: string): boolean {
  if (!value.startsWith(BraveWallet.FILECOIN_MAINNET) &&
    !value.startsWith(BraveWallet.FILECOIN_TESTNET)) {
    return false
  }
  // secp256k have 41 address length and BLS keys have 86 and FEVM f410 keys have 44
  return (value.length === 41 || value.length === 86 || value.length === 44)
}

/**
 * @deprecated Use isValidEVMAddress instead
 */
export function isValidAddress (value: string, length: number = 20): boolean {
  if (!value.match(/^0x[0-9A-Fa-f]*$/)) {
    return false
  }

  if (value.length !== 2 + 2 * length) {
    return false
  }

  return true
}

export function isValidEVMAddress (value: string): boolean {
  return /^0x[a-fA-F0-9]{40}$/.test(value)
}

export function isValidSolanaAddress (value: string): boolean {
  return /^[1-9A-HJ-NP-Za-km-z]{32,44}$/.test(value)
}

export const suggestNewAccountName = (
  accounts: BraveWallet.AccountInfo[],
  network: BraveWallet.NetworkInfo
) => {
  const accountTypeLength =
    accounts.filter((account) => account.accountId.coin === network.coin)
      .length + 1
  return `${network.symbolName} ${getLocale(
    'braveWalletAccount'
  )} ${accountTypeLength}`
}
