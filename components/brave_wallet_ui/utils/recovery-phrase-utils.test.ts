// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  cleanupRecoveryPhraseInput,
  isPhraseLengthValid,
  normalizeRecoveryPhraseInput
} from './recovery-phrase-utils'

describe('cleanupRecoveryPhraseInput', () => {
  it('prevents a space at the begining of the phrase', () => {
    expect(cleanupRecoveryPhraseInput(' phrase words here')).toBe(
      'phrase words here'
    )
  })

  it('removes periods', () => {
    expect(cleanupRecoveryPhraseInput('phrase. words. here.')).toBe(
      'phrase words here'
    )
  })

  it('prevents an extra space at the end of a 24 word phrase', () => {
    const recoveryWordsWithSpace = Array(24).fill('word ').join('')
    expect(cleanupRecoveryPhraseInput(recoveryWordsWithSpace)).toBe(
      recoveryWordsWithSpace.trim()
    )
  })
})

describe('isPhraseLengthValid', () => {
  it.each([12, 15, 18, 21, 24])(
    'length: %i should be valid',
    (length: number) => {
      expect(
        isPhraseLengthValid(Array(length).fill('word').join(' ')).isInvalid
      ).toBe(false)
    }
  )
  it.each([
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 16, 17, 19, 20, 22, 23, 25
  ])('length: %i should be invalid', (length: number) => {
    expect(
      isPhraseLengthValid(Array(length).fill('word').join(' ')).isInvalid
    ).toBe(true)
  })
})

describe('normalizeRecoveryPhraseInput', () => {
  it('should replace line breaks with a space', () => {
    const input = 'word1\r\nword2\nword3'
    const expectedOutput = 'word1 word2 word3'
    expect(normalizeRecoveryPhraseInput(input)).toBe(expectedOutput)
  })

  it('should replace multiple spaces with a single space', () => {
    const input = 'word1     word2   word3'
    const expectedOutput = 'word1 word2 word3'
    expect(normalizeRecoveryPhraseInput(input)).toBe(expectedOutput)
  })

  it('should trim leading and trailing whitespace', () => {
    const input = '   word1 word2 word3   '
    const expectedOutput = 'word1 word2 word3'
    expect(normalizeRecoveryPhraseInput(input)).toBe(expectedOutput)
  })

  it('should handle a combination of line breaks, multiple spaces, and leading/trailing whitespace', () => {
    const input = '\n  word1\r\n\r\n  word2   word3   \r\n'
    const expectedOutput = 'word1 word2 word3'
    expect(normalizeRecoveryPhraseInput(input)).toBe(expectedOutput)
  })

  it('should handle input that does not need normalization', () => {
    const input = 'word1 word2 word3'
    const expectedOutput = 'word1 word2 word3'
    expect(normalizeRecoveryPhraseInput(input)).toBe(expectedOutput)
  })

  it('should return an empty string when input is an empty string', () => {
    const input = ''
    const expectedOutput = ''
    expect(normalizeRecoveryPhraseInput(input)).toBe(expectedOutput)
  })

  it('should handle single words without modification', () => {
    const input = 'word1'
    const expectedOutput = 'word1'
    expect(normalizeRecoveryPhraseInput(input)).toBe(expectedOutput)
  })
})
