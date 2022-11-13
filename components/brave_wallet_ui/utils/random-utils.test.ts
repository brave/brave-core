// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { unbiasedRandom } from './random-utils'

describe('unbiasedRandom()', () => {
  test('generates a random number between 0 and 99', () => {
    const float = unbiasedRandom(0, 99)
    expect(float).toBeGreaterThanOrEqual(0)
    expect(float).toBeLessThanOrEqual(99)
  })

  test('generates a random number between 50 and 60', () => {
    const float = unbiasedRandom(50, 60)
    expect(float).toBeGreaterThanOrEqual(50)
    expect(float).toBeLessThanOrEqual(60)
  })
})
