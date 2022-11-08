// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '../../common/locale'
import { unbiasedRandom } from './random-utils'

export const ORDINALS = {
  0: getLocale('braveWalletOrdinalFirst'),
  1: getLocale('braveWalletOrdinalSecond'),
  2: getLocale('braveWalletOrdinalThird'),
  3: getLocale('braveWalletOrdinalFourth'),
  4: getLocale('braveWalletOrdinalFifth'),
  5: getLocale('braveWalletOrdinalSixth'),
  6: getLocale('braveWalletOrdinalSeventh'),
  7: getLocale('braveWalletOrdinalEighth'),
  8: getLocale('braveWalletOrdinalNinth'),
  9: getLocale('braveWalletOrdinalTenth'),
  10: getLocale('braveWalletOrdinalEleventh'),
  11: getLocale('braveWalletOrdinalTwelfth'),
  12: getLocale('braveWalletOridinalThirteenth'),
  13: getLocale('braveWalletOrdinalFourteenth'),
  14: getLocale('braveWalletOrdinalFifteenth'),
  15: getLocale('braveWalletOrdinalSixteenth'),
  16: getLocale('braveWalletOrdinalSeventeenth'),
  17: getLocale('braveWalletOrdinalEighteenth'),
  18: getLocale('braveWalletOrdinalNineteenth'),
  19: getLocale('braveWalletOrdinalTwentieth'),
  20: getLocale('braveWalletOrdinalTwentyFirst'),
  21: getLocale('braveWalletOrdinalTwentySecond'),
  22: getLocale('braveWalletOrdinalTwentyThird'),
  23: getLocale('braveWalletOrdinalTwentyFourth')
}

const suffixes = new Map([
  ['one', getLocale('braveWalletOrdinalSuffixOne')],
  ['two', getLocale('braveWalletOrdinalSuffixTwo')],
  ['few', getLocale('braveWalletOrdinalSuffixFew')],
  ['other', getLocale('braveWalletOrdinalSuffixOther')]
])

export const formatOrdinals = (n: number) => {
  const pr = new Intl.PluralRules(navigator.language, { type: 'ordinal' })
  const rule = pr.select(n)
  const suffix = suffixes.get(rule)
  return `${n}${suffix}`
}

export const getWordIndicesToVerfy = (_wordsLength: number): number[] => {
  if (_wordsLength < 3) {
    return [-3, -2, -1] // phrase is not long enough (must be longer than 3 words)
  }

  // limit randomness to first 24 words
  const wordsLength = _wordsLength > 24 ? 24 : _wordsLength

  // get next random index
  const indicesSet = new Set<number>([])

  while (indicesSet.size < 3) {
    const nextIndex = unbiasedRandom(0, wordsLength - 1)
    indicesSet.add(nextIndex)
  }

  return Array.from(indicesSet).sort((a, b) => a - b) // verify in order
}
