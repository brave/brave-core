// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '../../common/locale'
import { unbiasedRandom } from './random-utils'

export const ORDINALS = {
  0: getLocale(S.BRAVE_WALLET_ORDINAL_FIRST),
  1: getLocale(S.BRAVE_WALLET_ORDINAL_SECOND),
  2: getLocale(S.BRAVE_WALLET_ORDINAL_THIRD),
  3: getLocale(S.BRAVE_WALLET_ORDINAL_FOURTH),
  4: getLocale(S.BRAVE_WALLET_ORDINAL_FIFTH),
  5: getLocale(S.BRAVE_WALLET_ORDINAL_SIXTH),
  6: getLocale(S.BRAVE_WALLET_ORDINAL_SEVENTH),
  7: getLocale(S.BRAVE_WALLET_ORDINAL_EIGHTH),
  8: getLocale(S.BRAVE_WALLET_ORDINAL_NINTH),
  9: getLocale(S.BRAVE_WALLET_ORDINAL_TENTH),
  10: getLocale(S.BRAVE_WALLET_ORDINAL_ELEVENTH),
  11: getLocale(S.BRAVE_WALLET_ORDINAL_TWELFTH),
  12: getLocale(S.BRAVE_WALLET_ORDINAL_THIRTEENTH),
  13: getLocale(S.BRAVE_WALLET_ORDINAL_FOURTEENTH),
  14: getLocale(S.BRAVE_WALLET_ORDINAL_FIFTEENTH),
  15: getLocale(S.BRAVE_WALLET_ORDINAL_SIXTEENTH),
  16: getLocale(S.BRAVE_WALLET_ORDINAL_SEVENTEENTH),
  17: getLocale(S.BRAVE_WALLET_ORDINAL_EIGHTEENTH),
  18: getLocale(S.BRAVE_WALLET_ORDINAL_NINETEENTH),
  19: getLocale(S.BRAVE_WALLET_ORDINAL_TWENTIETH),
  20: getLocale(S.BRAVE_WALLET_ORDINAL_TWENTY_FIRST),
  21: getLocale(S.BRAVE_WALLET_ORDINAL_TWENTY_SECOND),
  22: getLocale(S.BRAVE_WALLET_ORDINAL_TWENTY_THIRD),
  23: getLocale(S.BRAVE_WALLET_ORDINAL_TWENTY_FOURTH),
}

const suffixes = new Map([
  ['one', getLocale(S.BRAVE_WALLET_ORDINAL_SUFFIX_ONE)],
  ['two', getLocale(S.BRAVE_WALLET_ORDINAL_SUFFIX_TWO)],
  ['few', getLocale(S.BRAVE_WALLET_ORDINAL_SUFFIX_FEW)],
  ['other', getLocale(S.BRAVE_WALLET_ORDINAL_SUFFIX_OTHER)],
])

export const formatOrdinals = (n: number) => {
  const pr = new Intl.PluralRules(navigator.language, { type: 'ordinal' })
  const rule = pr.select(n)
  const suffix = suffixes.get(rule)
  return `${n}${suffix}`
}

export const getWordIndicesToVerify = (_wordsLength: number): number[] => {
  if (_wordsLength < 3) {
    // phrase is not long enough (must be longer than 3 words)
    return [-3, -2, -1]
  }

  // limit randomness to first 24 words
  const wordsLength = _wordsLength > 24 ? 24 : _wordsLength

  // get next random index
  const indicesSet = new Set<number>([])

  while (indicesSet.size < 3) {
    const nextIndex = unbiasedRandom(0, wordsLength - 1)
    indicesSet.add(nextIndex)
  }

  return Array.from(indicesSet)
}
