// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { FuzzyFinder } from './fuzzy_finder'

describe('FuzzyFinder', () => {
  test('NonmatchIsZero', () => {
    const finder1 = new FuzzyFinder('orange')
    const result1 = finder1.find('orangutan')
    expect(result1.score).toBe(0)
    expect(result1.ranges).toEqual([])

    const finder2 = new FuzzyFinder('elephant')
    const result2 = finder2.find('orangutan')
    expect(result2.score).toBe(0)
    expect(result2.ranges).toEqual([])
  })

  test('ExactMatchIsOne', () => {
    const finder = new FuzzyFinder('orange')
    const result = finder.find('orange')
    expect(result.score).toBe(1)
    expect(result.ranges).toEqual([{ start: 0, end: 6 }])
  })

  // This ensures coverage for a fast path. Successful match is
  // tested in ExactMatchIsOne() above.
  test('NeedleHaystackSameLength', () => {
    const finder = new FuzzyFinder('ranges')
    const result = finder.find('orange')
    expect(result.score).toBe(0)
    expect(result.ranges).toEqual([])
  })

  // This ensures coverage for a fast path (just making sure the path has
  // coverage rather than ensuring the path is taken).
  test('SingleCharNeedle', () => {
    const finder = new FuzzyFinder('o')

    const prefixResult = finder.find('orange')
    expect(prefixResult.ranges).toEqual([{ start: 0, end: 1 }])
    const prefixScore = prefixResult.score

    const internalResult = finder.find('phone')
    expect(internalResult.ranges).toEqual([{ start: 2, end: 3 }])
    const internalScore = internalResult.score

    const boundaryResult = finder.find('phone operator')
    expect(boundaryResult.ranges).toEqual([{ start: 6, end: 7 }])
    const boundaryScore = boundaryResult.score

    // Expected ordering:
    // - Prefix should rank highest.
    // - Word boundary matches that are not the prefix should rank next
    // highest, even if there's an internal match earlier in the haystack.
    // - Internal matches should rank lowest.
    expect(prefixScore).toBeGreaterThan(boundaryScore)
    expect(boundaryScore).toBeGreaterThan(internalScore)

    // ...and non-matches should have score = 0.
    const noMatchResult = finder.find('aquarium')
    expect(noMatchResult.score).toBe(0)
    expect(noMatchResult.ranges).toEqual([])
  })

  test('CaseInsensitive', () => {
    const finder = new FuzzyFinder('orange')
    const result = finder.find('Orange')
    expect(result.score).toBe(1)
    expect(result.ranges).toEqual([{ start: 0, end: 6 }])
  })

  test('PrefixRanksHigherThanInternal', () => {
    const finder = new FuzzyFinder('orange')
    const prefixResult = finder.find('Orange juice')
    const nonPrefixResult = finder.find('William of Orange')

    const prefixRank = prefixResult.score
    const nonPrefixRank = nonPrefixResult.score

    expect(prefixRank).toBeGreaterThan(0)
    expect(nonPrefixRank).toBeGreaterThan(0)
    expect(prefixRank).toBeLessThan(1)
    expect(nonPrefixRank).toBeLessThan(1)
    expect(prefixRank).toBeGreaterThan(nonPrefixRank)
  })

  test('NeedleLongerThanHaystack', () => {
    const finder = new FuzzyFinder('orange juice')
    const result = finder.find('orange')
    expect(result.score).toBe(0)
    expect(result.ranges).toEqual([])
  })

  test('Noncontiguous', () => {
    const finder = new FuzzyFinder('tuot')
    const result = finder.find('Tlön, Uqbar, Orbis Tertius')
    expect(result.score).toBeGreaterThan(0)
    expect(result.ranges).toEqual([
      { start: 0, end: 1 },
      { start: 6, end: 7 },
      { start: 13, end: 14 },
      { start: 19, end: 20 },
    ])
  })
})
