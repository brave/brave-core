// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { FuzzyFinder, Match, isPrefixMatch } from './fuzzy_finder'

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

/** An `@` mention that matches the query, and whether it's of a connector. */
export interface RankedMention {
  isConnector: boolean
  match: Match
}

/**
 * Orders the mentions that match the query, best first. A connector comes
 * before any content that it matches at least as well as, and an exact or
 * prefix match on a connector's name comes before everything else.
 */
export function compareMentions(a: RankedMention, b: RankedMention): number {
  const aLeads = a.isConnector && isPrefixMatch(a.match)
  const bLeads = b.isConnector && isPrefixMatch(b.match)
  if (aLeads !== bLeads) {
    return aLeads ? -1 : 1
  }
  if (a.match.score !== b.match.score) {
    return b.match.score - a.match.score
  }
  if (a.isConnector !== b.isConnector) {
    return a.isConnector ? -1 : 1
  }
  return 0
}
