/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
  WalletAccountType
} from '../constants/types'

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
