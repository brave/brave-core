// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getModelIcon } from './constants'

describe('getModelIcon', () => {
  it('Should return fallback icon for unknown model keys', () => {
    expect(getModelIcon('unknown-model')).toBe('product-brave-leo')
    expect(getModelIcon('')).toBe('product-brave-leo')
    expect(getModelIcon('chat-invalid')).toBe('product-brave-leo')
  })
})
