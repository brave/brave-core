// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BLOCKED_COMPARISONS,
  Comparison,
  DATA_COMPARISONS,
  TIME_COMPARISONS,
  pickComparison,
} from './comparisons'

const MINUTE_MS = 60 * 1000
const HOUR_MS = 60 * MINUTE_MS
const KB = 1024
const GB = 1024 * 1024 * KB

describe('brave recap comparisons', () => {
  it.each([
    ['blocked', BLOCKED_COMPARISONS],
    ['time', TIME_COMPARISONS],
    ['data', DATA_COMPARISONS],
  ])('keeps the %s milestones sorted and unique', (_, table: Comparison[]) => {
    const froms = table.map((comparison) => comparison.from)
    expect(froms).toEqual([...new Set(froms)].sort((a, b) => a - b))
  })

  it('picks the highest milestone reached', () => {
    expect(pickComparison(BLOCKED_COMPARISONS, 274_184)).toBe(
      'BRAVE_RECAP_BLOCKED_COMPARISON_270000',
    )
    expect(pickComparison(TIME_COMPARISONS, 3.6 * HOUR_MS)).toBe(
      'BRAVE_RECAP_TIME_COMPARISON_3H30M',
    )
    expect(pickComparison(DATA_COMPARISONS, 8.83 * GB)).toBe(
      'BRAVE_RECAP_DATA_COMPARISON_8GB',
    )
  })

  it('applies a milestone from its exact value', () => {
    expect(pickComparison(BLOCKED_COMPARISONS, 270_000)).toBe(
      'BRAVE_RECAP_BLOCKED_COMPARISON_270000',
    )
    expect(pickComparison(BLOCKED_COMPARISONS, 269_999)).toBe(
      'BRAVE_RECAP_BLOCKED_COMPARISON_250000',
    )
  })

  it('has nothing below the first milestone', () => {
    expect(pickComparison(TIME_COMPARISONS, 29_999)).toBeUndefined()
    expect(pickComparison(DATA_COMPARISONS, 99 * KB)).toBeUndefined()
  })

  it('keeps the last milestone above the table', () => {
    expect(pickComparison(BLOCKED_COMPARISONS, 50_000_000)).toBe(
      'BRAVE_RECAP_BLOCKED_COMPARISON_2000000',
    )
    expect(pickComparison(TIME_COMPARISONS, 1000 * HOUR_MS)).toBe(
      'BRAVE_RECAP_TIME_COMPARISON_30H',
    )
  })
})
