// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { describe, expect, it } from '@jest/globals'

import { getNemotronModelType } from './utils'

describe('getNemotronModelType', () => {
  it.each(['en-US', 'en-GB', 'en-AU', 'en', 'EN-gb'])(
    'picks the English model for %s',
    (lang) => {
      expect(getNemotronModelType(lang)).toEqual({
        modelType: 'english',
        promptId: null,
      })
    },
  )

  it.each([
    ['es-ES', 2],
    ['es-US', 3],
    ['ES-us', 3],
    ['pt-PT', 13],
    ['hi-IN', 6],
  ])('picks the multilingual model and prompt for %s', (lang, promptId) => {
    expect(getNemotronModelType(lang)).toEqual({
      modelType: 'multilingual',
      promptId,
    })
  })

  // Includes near misses of supported tags, which neither model takes.
  it.each(['es-MX', 'es', 'hi', 'fr-FR', 'english', ''])(
    'rejects %s',
    (lang) => {
      expect(() => getNemotronModelType(lang)).toThrow(
        'Unsupported ASR language',
      )
    },
  )
})
