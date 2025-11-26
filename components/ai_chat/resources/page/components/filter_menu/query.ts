// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

function normalizeText(text: string) {
  return text.trim().replace(/\s/g, '').toLocaleLowerCase()
}

export function matches(query: string, text: string) {
  // Note: Empty string should match anything.
  if (query === '') {
    return 0
  }

  return normalizeText(text).indexOf(normalizeText(query))
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
