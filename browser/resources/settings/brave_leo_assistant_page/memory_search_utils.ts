/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Helpers for the search box over the memories that the user wrote and the
// memories that Leo learned. Both lists filter and highlight with them, so that
// a word is a match in both or in neither.

export interface TextSegment {
  text: string
  match: boolean
}

function getPattern(query: string, flags: string): RegExp | null {
  const trimmed = query.trim()
  if (!trimmed) {
    return null
  }
  return new RegExp(trimmed.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), flags)
}

// Whether `text` contains `query`, ignoring case. An empty query matches
// everything.
export function matchesQuery(text: string, query: string): boolean {
  const pattern = getPattern(query, 'i')
  return !pattern || pattern.test(text)
}

// Cuts `text` into the parts that are `query` (ignoring case) and the parts
// between them, in order, so that the page can mark the first kind.
export function splitByQuery(text: string, query: string): TextSegment[] {
  const pattern = getPattern(query, 'gi')
  if (!pattern) {
    return [{ text, match: false }]
  }
  const segments: TextSegment[] = []
  let end = 0
  for (const found of text.matchAll(pattern)) {
    const start = found.index ?? 0
    if (start > end) {
      segments.push({ text: text.slice(end, start), match: false })
    }
    segments.push({ text: found[0], match: true })
    end = start + found[0].length
  }
  if (end < text.length || segments.length === 0) {
    segments.push({ text: text.slice(end), match: false })
  }
  return segments
}
