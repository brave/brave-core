// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { FuzzyFinder } from './fuzzy_finder'

export function matches(finder: FuzzyFinder, text: string) {
  // Note: Empty string should match anything.
  if (finder.needle === '') {
    return { score: 0, ranges: [] }
  }

  const result = finder.find(text)
  if (result.ranges.length === 0) {
    return undefined
  }
  return result
}

export function extractQuery(
  input: string,
  options: {
    onlyAtStart: boolean
    triggerCharacter: string
  },
) {
  const regex = options.onlyAtStart
    ? new RegExp(`^${options.triggerCharacter}(.*)`)
    : new RegExp(`${options.triggerCharacter}(.*)`)

  const match = input.match(regex)
  return match ? match[1] : null
}

export function useExtractedQuery(
  input: string,
  options: {
    onlyAtStart: boolean
    triggerCharacter: string
  },
) {
  return React.useMemo(
    () => extractQuery(input, options),
    [input, options.onlyAtStart, options.triggerCharacter],
  )
}
