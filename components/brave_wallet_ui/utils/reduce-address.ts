// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const prefixes = [
  // Ethereum
  '0x',

  // Cardano
  'addr1',
  'addr_test1',

  // Bitcoin
  'bc1q',
  'bc1p',
  'tb1q',
  'tb1p',

  // Zcash
  'u1',
  'utest1',
  't1',
  't2',
  't3',
  'tm',

  // Filecoin
  'f1',
  'f3',
  'f410',
  't1',
  't3',
  't410',
]

export const reduceAddress = (
  address?: string,
  fillerChars?: string,
  length?: number,
) => {
  if (!address) {
    return ''
  }

  const frontSliceLength = length ?? 4
  const backSliceLength = length ?? 4
  const frontPrefixLength =
    prefixes.find((p) => {
      return address.toLowerCase().startsWith(p)
    })?.length ?? 0

  const firstHalf = address.slice(0, frontSliceLength + frontPrefixLength)
  const secondHalf = address.slice(-backSliceLength)

  return firstHalf.concat(fillerChars || '***', secondHalf)
}
