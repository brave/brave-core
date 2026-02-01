// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'

export const U128_MAX = BigInt('0xffffffffffffffffffffffffffffffff')

export function bigIntToUint128(amount: bigint) {
  if (amount > U128_MAX) {
    throw new Error('numeric limits exceeded for amount u128')
  }

  const high = amount >> BigInt(64)
  const low = amount & BigInt('0xffffffffffffffff')

  const sendAmount = new BraveWallet.uint128()
  sendAmount.high = high
  sendAmount.low = low

  return sendAmount
}

export function Uint128ToBigInt(amount: BraveWallet.uint128 | undefined) {
  if (!amount) return
  const high = amount.high
  const low = amount.low
  return (high << BigInt(64)) | low
}
