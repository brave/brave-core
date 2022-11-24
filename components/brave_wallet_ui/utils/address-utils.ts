/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// utils
import { getLocale } from '../../common/locale'

// types
import {
  BraveWallet,
  WalletAccountType
} from '../constants/types'

export function isValidFilAddress (value: string): boolean {
  if (!value.startsWith(BraveWallet.FILECOIN_MAINNET) &&
    !value.startsWith(BraveWallet.FILECOIN_TESTNET)) {
    return false
  }
  // secp256k have 41 address length and BLS keys have 86
  return (value.length === 41 || value.length === 86)
}

export function isValidAddress (value: string, length: number): boolean {
  if (!value.match(/^0x[0-9A-Fa-f]*$/)) {
    return false
  }

  if (value.length !== 2 + 2 * length) {
    return false
  }

  return true
}

export function isHardwareAccount (accounts: WalletAccountType[], address: string) {
  return accounts.some(account => account.deviceId && account.address === address)
}

export const suggestNewAccountName = (
  accounts: WalletAccountType[],
  network: BraveWallet.NetworkInfo
) => {
  const accountTypeLength = accounts.filter((account) => account.coin === network.coin).length + 1
  return `${network.symbolName} ${getLocale('braveWalletAccount')} ${accountTypeLength}`
}
