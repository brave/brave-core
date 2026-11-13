// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { describe, expect, it } from '@jest/globals'

import { getNemotronModelType } from './utils'

describe('getNemotronModelType', () => {
  it.each(['en-US'])('picks the English model for %s', (lang) => {
    expect(getNemotronModelType(lang)).toEqual({
      modelType: 'english',
      promptId: null,
    })
  })

  it.each([
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
  ])('picks the multilingual model and prompt for %s', (lang, promptId) => {
    expect(getNemotronModelType(lang)).toEqual({
      modelType: 'multilingual',
      promptId,
    })
  })

  // The browser sends only canonical tags, so the worker takes no near miss,
  // such as the model card's 'ar-AR' label.
  it.each(['es-MX', 'es', 'hi', 'ar-AR', 'zh-TW', 'th-TH', 'english', ''])(
    'rejects %s',
    (lang) => {
      expect(() => getNemotronModelType(lang)).toThrow(
        'Unsupported ASR language',
      )
    },
  )
})
