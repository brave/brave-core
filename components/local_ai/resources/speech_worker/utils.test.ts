// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { describe, expect, it } from '@jest/globals'

import { findLanguageTagTokenIds, getNemotronLocale } from './utils'

describe('getNemotronLocale', () => {
  it('serves en-US on the English model with no prompt', () => {
    expect(getNemotronLocale('english', 'en-US')).toEqual({
      model: 'english',
      promptId: null,
    })
  })

  // The English model serves only en-US.
  it.each(['de-DE', 'es-US'])(
    'rejects %s on the English model',
    (lang: string) => {
      expect(() => getNemotronLocale('english', lang)).toThrow(
        'Unsupported ASR language',
      )
    },
  )

  it.each([
    ['en-US', 0],
    ['ar', 7],
    ['de-DE', 9],
    ['es-ES', 2],
    ['es-US', 3],
    ['fr-FR', 8],
    ['fr-CA', 100],
    ['hi-IN', 6],
    ['it-IT', 15],
    ['ja-JP', 10],
    ['ko-KR', 14],
    ['nl-NL', 16],
    ['pt-BR', 12],
    ['pt-PT', 13],
    ['ru-RU', 11],
    ['tr-TR', 18],
    ['uk-UA', 19],
    ['vi-VN', 33],
  ])(
    'picks the prompt of %s on the multilingual model',
    (lang: string, promptId: number) => {
      expect(getNemotronLocale('multilingual', lang)).toEqual({
        model: 'multilingual',
        promptId,
      })
    },
  )

  // The browser sends only canonical tags, so the worker takes no near miss,
  // such as the model card's 'ar-AR' label.
  it.each(['es-MX', 'es', 'hi', 'ar-AR', 'zh-TW', 'th-TH', 'english', ''])(
    'rejects %s',
    (lang: string) => {
      expect(() => getNemotronLocale('multilingual', lang)).toThrow(
        'Unsupported ASR language',
      )
    },
  )
})

describe('findLanguageTagTokenIds', () => {
  it('finds language tag token IDs', () => {
    const tokens = [
      '<unk>',
      '▁hello',
      '<es-ES>',
      '▁world',
      '<fr-FR>',
      '<en-US>',
    ]

    expect(findLanguageTagTokenIds(tokens)).toEqual(new Set([2, 4, 5]))
  })

  it('returns an empty set when no language tags exist', () => {
    const tokens = ['<unk>', '▁hello', '▁world', 'ing']

    expect(findLanguageTagTokenIds(tokens)).toEqual(new Set<number>())
  })

  it('ignores tokens that do not match the language tag pattern', () => {
    const tokens = [
      '<unk>',
      '<es>',
      '<es-es>',
      '<ES-ES>',
      '<es-ES-extra>',
      'prefix<es-ES>',
      '<es-ES>suffix',
      '<es-ES>',
    ]

    expect(findLanguageTagTokenIds(tokens)).toEqual(new Set([7]))
  })
})
