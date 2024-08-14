// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

export const WORD_SEPARATOR = ' '

export const isPhraseLengthValid = (phrase: string) => {
  const wordsInPhraseValue = phrase.trim().split(/\s+/g).length
  // valid lengths: 12, 15, 18, 21, or 24
  const isInvalid =
    wordsInPhraseValue < 12 ||
    (wordsInPhraseValue > 12 && wordsInPhraseValue < 15) ||
    (wordsInPhraseValue > 15 && wordsInPhraseValue < 18) ||
    (wordsInPhraseValue > 18 && wordsInPhraseValue < 21) ||
    (wordsInPhraseValue > 21 && wordsInPhraseValue < 24) ||
    wordsInPhraseValue > 24

  return { isInvalid, wordsInPhraseValue }
}

export const cleanupRecoveryPhraseInput = (value: string) => {
  // This prevents a space at the begining of the phrase.
  const removedBeginingWhiteSpace = value.trimStart()

  // macOS automatically replaces double-spaces with a period.
  const removePeriod = removedBeginingWhiteSpace.replace(/['/.']/g, '')

  // max length
  const maxLength = 24

  // This prevents an extra space at the end of a 24 word phrase.
  const needsCleaning =
    removedBeginingWhiteSpace.split(' ').length >= maxLength + 1

  const cleanedInput = needsCleaning ? removePeriod.trimEnd() : removePeriod

  return cleanedInput
}

export const normalizeRecoveryPhraseInput = (value: string) => {
  return value
    .replace(/[\r\n]+/g, WORD_SEPARATOR) // replace \r and \n with a space
    .replace(/\s+/g, WORD_SEPARATOR) // replace multiple spaces with a single space
    .trim()
}
