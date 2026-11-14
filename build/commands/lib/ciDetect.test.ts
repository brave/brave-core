// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { detectCI } from './ciDetect.ts'

describe('detectCI', () => {
  it('is false locally', () => {
    expect(detectCI({})).toBe(false)
  })

  it.each([{ BUILD_ID: '177' }, { TEAMCITY_VERSION: '2025.1' }])(
    'is true on CI (%o)',
    (env) => {
      expect(detectCI(env)).toBe(true)
    },
  )

  it('is false under the recipes engine, even on Jenkins', () => {
    expect(detectCI({ BUILD_ID: '177', BRAVE_RECIPES_ENGINE: '1' })).toBe(false)
  })
})
