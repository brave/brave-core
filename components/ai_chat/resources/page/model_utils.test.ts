// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../common/mojom'
import { DEFAULT_MAX_INPUT_CHAR, getMaxInputCharLimit } from './model_utils'

function leoModel(maxAssociatedContentLength: number): Mojom.Model {
  return {
    options: {
      leoModelOptions: { maxAssociatedContentLength },
      customModelOptions: undefined,
    },
  } as unknown as Mojom.Model
}

function customModel(maxAssociatedContentLength: number): Mojom.Model {
  return {
    options: {
      leoModelOptions: undefined,
      customModelOptions: { maxAssociatedContentLength },
    },
  } as unknown as Mojom.Model
}

describe('getMaxInputCharLimit', () => {
  it('returns the default limit when there is no model', () => {
    expect(getMaxInputCharLimit(undefined)).toBe(DEFAULT_MAX_INPUT_CHAR)
  })

  it('returns the default limit for Leo models regardless of context', () => {
    expect(getMaxInputCharLimit(leoModel(999999))).toBe(DEFAULT_MAX_INPUT_CHAR)
  })

  it('scales the limit with a custom model context size', () => {
    // Large-context BYOM model should not be capped by the default limit.
    expect(getMaxInputCharLimit(customModel(800000))).toBe(800000)
  })

  it('never drops below the default limit for small custom models', () => {
    expect(getMaxInputCharLimit(customModel(5000))).toBe(DEFAULT_MAX_INPUT_CHAR)
  })
})
