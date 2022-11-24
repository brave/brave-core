// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  cleanupRecoveryPhraseInput,
  isPhraseLengthValid
} from './recovery-phrase-utils'

describe('cleanupRecoveryPhraseInput', () => {
  it('prevents a space at the begining of the phrase', () => {
    expect(cleanupRecoveryPhraseInput(' phrase words here')).toBe('phrase words here')
  })

  it('removes periods', () => {
    expect(cleanupRecoveryPhraseInput('phrase. words. here.')).toBe('phrase words here')
  })

  it('prevents an extra space at the end of a 24 word phrase', () => {
    expect(cleanupRecoveryPhraseInput('word word word word word word word word word word word word word word word word word word word word word word word word '))
      .toBe('word word word word word word word word word word word word word word word word word word word word word word word word')
  })
})

describe('isPhraseLengthValid', () => {
  it.each([12, 15, 18, 21, 24])('length: %i should be valid', (length: number) => {
    expect(isPhraseLengthValid(Array(length).fill('word').join(' ')).isInvalid).toBe(false)
  })
  it.each([
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 13, 14, 16, 17, 19,
    20, 22, 23, 25
  ])('length: %i should be invalid', (length: number) => {
    expect(isPhraseLengthValid(Array(length).fill('word').join(' ')).isInvalid).toBe(true)
  })
})
