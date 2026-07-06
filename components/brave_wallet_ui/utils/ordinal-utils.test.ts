// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mockRecoveryPhrase } from '../stories/mock-data/user-accounts'
import { getWordIndicesToVerify } from './ordinal-utils'
import * as randomUtils from './random-utils'

describe('getWordIndicesToVerify', () => {
  it('creates a list 3 of unique numbers for 12-word phrases', () => {
    const indices = getWordIndicesToVerify(mockRecoveryPhrase.length)
    expect(new Set(indices).size).toBe(3)
  })

  it('creates a list of 3 unique numbers for 24-word phrases', () => {
    const indices = getWordIndicesToVerify(24)
    expect(new Set(indices).size).toBe(3)
    expect(indices.every((index) => index >= 0 && index < 24)).toBe(true)
  })

  it('returns indices in randomized order', () => {
    const randomSpy = jest
      .spyOn(randomUtils, 'unbiasedRandom')
      .mockReturnValueOnce(10)
      .mockReturnValueOnce(2)
      .mockReturnValueOnce(5)

    expect(getWordIndicesToVerify(24)).toEqual([10, 2, 5])

    randomSpy.mockRestore()
  })
})
