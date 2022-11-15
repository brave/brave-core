// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mockRecoveryPhrase } from '../stories/mock-data/user-accounts'
import { getWordIndicesToVerfy } from './ordinal-utils'

describe('getWordIndicesToVerfy', () => {
  it('creates a list 3 of unique numbers', () => {
    const indices = getWordIndicesToVerfy(mockRecoveryPhrase.length)
    expect(new Set(indices).size).toBe(3)
  })
})
