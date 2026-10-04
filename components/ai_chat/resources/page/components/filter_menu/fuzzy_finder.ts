// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This code has been ported to Typescript from browser/ui/commander/fuzzy_finder.h

// Used only for exact matches.
const MAX_SCORE = 1.0
// When needle is a prefix of haystack.
const PREFIX_SCORE = 0.99
// When a heuristic determines that the match should score highly,
// but it is *not* an exact match or prefix.
const VERY_HIGH_SCORE = 0.95

// Max haystack size in UTF-16 units for the dynamic programming algorithm.
// Haystacks longer than this are scored by ConsecutiveMatchWithGaps.
const MAX_HAYSTACK = 1024

// Max needle size in UTF-16 units for the dynamic programming algorithm.
// Needles longer than this are scored by ConsecutiveMatchWithGaps
const MAX_NEEDLE = 16

class MatchRecord {
  constructor(
    public start: number,
    public end: number,
    public length: number,
    public isBoundary: boolean,
    public gapBefore: number,
  ) {}
}

export interface Match {
  score: number
  ranges: Range[]
}

interface Range {
  start: number
  end: number
}

// Scores matches identified by consecutiveMatchWithGaps()
function scoreForMatches(
  matches: MatchRecord[],
  needleSize: number,
  haystackSize: number,
): number {
  // base_score is the maximum per match, so total should not exceed 1.0
  const baseScore = 1.0 / needleSize
  const gapPenalty = 1.0 / haystackSize
  const REGULAR_MULTIPLIER = 0.5
  const WORD_BOUNDARY_MULTIPLIER = 0.8
  const INITIAL_MULTIPLIER = 1.0
  let score = 0

  for (let i = 0; i < matches.length; i++) {
    const match = matches[i]
    // The first character of the match is special; it gets a relative bonus
    // if it is on a boundary. Otherwise, it is penalized by the distance
    // between it and the previous match.
    if (match.isBoundary) {
      score +=
        baseScore * (i === 0 ? INITIAL_MULTIPLIER : WORD_BOUNDARY_MULTIPLIER)
    } else {
      const penaltyMultiplier = 1 - gapPenalty * match.gapBefore
      console.assert(penaltyMultiplier > 0)
      score += baseScore * REGULAR_MULTIPLIER * penaltyMultiplier
    }
    // ...then the rest of a contiguous match.
    score += (match.length - 1) * baseScore * REGULAR_MULTIPLIER
  }
  console.assert(score <= 1.0)
  return score
}

function lengthInCodePoints(str: string): number {
  return [...str].length
}

// Returns a positive score if every code point in needle is present in
// haystack in the same order. The match *need not* be contiguous. Matches in
// special positions are given extra weight, and noncontiguous matches are
// penalized based on the size of the gaps between.
function consecutiveMatchWithGaps(
  needle: string,
  haystack: string,
  matchedRanges: Range[],
): number {
  console.assert(needle === needle.toLowerCase())
  console.assert(haystack === haystack.toLowerCase())
  console.assert(matchedRanges.length === 0)

  // Special case for prefix
  if (haystack.startsWith(needle)) {
    matchedRanges.push({ start: 0, end: needle.length })
    return PREFIX_SCORE
  }

  const needleChars = [...needle]
  const haystackChars = [...haystack]

  const matches: MatchRecord[] = []
  let gapSizeBeforeMatch = 0
  let matchBeganOnBoundary = true
  let matchStart = -1
  let matchLength = 0
  let nIdx = 0
  let hIdx = 0
  let hCharOffset = 0

  // Find matching ranges
  while (nIdx < needleChars.length && hIdx < haystackChars.length) {
    if (needleChars[nIdx] === haystackChars[hIdx]) {
      // There's a match
      if (matchLength === 0) {
        // Match start
        matchStart = hCharOffset
        matchBeganOnBoundary = hIdx === 0 || /\s/.test(haystackChars[hIdx - 1])
      }
      matchLength++
      hCharOffset += haystackChars[hIdx].length
      hIdx++
      nIdx++
    } else {
      if (matchLength > 0) {
        console.assert(matchStart !== -1)
        matches.push(
          new MatchRecord(
            matchStart,
            hCharOffset,
            matchLength,
            matchBeganOnBoundary,
            gapSizeBeforeMatch,
          ),
        )
        matchLength = 0
        gapSizeBeforeMatch = 1
        matchStart = -1
      } else {
        gapSizeBeforeMatch++
      }
      hCharOffset += haystackChars[hIdx].length
      hIdx++
    }
  }

  if (nIdx < needleChars.length) {
    // Didn't match all of needle
    matchedRanges.length = 0
    return 0
  }

  if (matchLength > 0) {
    console.assert(matchStart !== -1)
    matches.push(
      new MatchRecord(
        matchStart,
        hCharOffset,
        matchLength,
        matchBeganOnBoundary,
        gapSizeBeforeMatch,
      ),
    )
  }

  for (const match of matches) {
    matchedRanges.push({ start: match.start, end: match.end })
  }

  let score = scoreForMatches(
    matches,
    lengthInCodePoints(needle),
    lengthInCodePoints(haystack),
  )
  score *= PREFIX_SCORE // Normalize so that a prefix always wins
  return score
}

// Converts a list of indices in positions into contiguous ranges
function convertPositionsToRanges(
  positions: number[],
  matchedRanges: Range[],
): void {
  const n = positions.length
  console.assert(n > 0)
  let start = positions[0]
  let length = 1

  for (let i = 0; i < n - 1; i++) {
    if (positions[i] + 1 < positions[i + 1]) {
      // Noncontiguous positions -> close out the range
      matchedRanges.push({ start, end: start + length })
      start = positions[i + 1]
      length = 1
    } else {
      length++
    }
  }
  matchedRanges.push({ start, end: start + length })
}

// Returns the maximum score for the given matrix, then backtracks to fill in
// matchedRanges
function scoreForMatrix(
  scoreMatrix: Int32Array,
  width: number,
  height: number,
  codepointToOffset: Uint32Array,
  matchedRanges: Range[],
): number {
  // Find winning score and its index
  let maxIndex = 0
  let maxScore = 0

  for (let i = 0; i < width; i++) {
    const score = scoreMatrix[(height - 1) * width + i]
    if (score > maxScore) {
      maxScore = score
      maxIndex = i
    }
  }

  // Backtrack through the matrix to find matching positions
  const positions: number[] = [codepointToOffset[maxIndex]]
  let curI = maxIndex
  let curJ = height - 1

  while (curJ > 0) {
    // Move diagonally...
    curI--
    curJ--
    // ...then scan left until the score stops increasing
    let current = scoreMatrix[curJ * width + curI]
    let left = curI === 0 ? 0 : scoreMatrix[curJ * width + curI - 1]

    while (current < left) {
      curI--
      if (curI === 0) {
        break
      }
      current = left
      left = scoreMatrix[curJ * width + curI - 1]
    }
    positions.push(codepointToOffset[curI])
  }

  positions.reverse()
  convertPositionsToRanges(positions, matchedRanges)
  return maxScore
}

export class FuzzyFinder {
  needle: string
  private scoreMatrix: Int32Array
  private consecutiveMatrix: Int32Array
  private wordBoundaries: boolean[]
  private codepointToOffset: Uint32Array

  constructor(needle: string) {
    this.needle = needle.toLowerCase()
    this.scoreMatrix = new Int32Array(0)
    this.consecutiveMatrix = new Int32Array(0)
    this.wordBoundaries = []
    this.codepointToOffset = new Uint32Array(0)

    if (this.needle.length <= MAX_NEEDLE) {
      const capacity = this.needle.length * MAX_HAYSTACK
      this.scoreMatrix = new Int32Array(capacity)
      this.consecutiveMatrix = new Int32Array(capacity)
    }
  }

  find(haystack: string): Match {
    const matchedRanges: Range[] = []
    const folded = haystack.toLowerCase()
    const m = this.needle.length
    const n = folded.length

    // Special case 0: M > N. We don't allow skipping anything in needle
    if (m > n) {
      return { score: 0, ranges: [] }
    }

    // Special case 1: M == N. Must be either exact match or non-match
    if (m === n) {
      if (folded === this.needle) {
        matchedRanges.push({ start: 0, end: this.needle.length })
        return { score: MAX_SCORE, ranges: matchedRanges }
      } else {
        return { score: 0, ranges: [] }
      }
    }

    // Special case 2: needle is a prefix of haystack
    if (folded.startsWith(this.needle)) {
      matchedRanges.push({ start: 0, end: this.needle.length })
      return { score: PREFIX_SCORE, ranges: matchedRanges }
    }

    // Special case 3: M == 1
    if (m === 1) {
      let substringPosition = folded.indexOf(this.needle)
      while (substringPosition !== -1) {
        if (substringPosition === 0) {
          // Prefix match
          matchedRanges.push({ start: 0, end: 1 })
          return { score: PREFIX_SCORE, ranges: matchedRanges }
        } else {
          const previous = folded[substringPosition - 1]
          if (/\s/.test(previous)) {
            // Word boundary
            matchedRanges.length = 0
            matchedRanges.push({
              start: substringPosition,
              end: substringPosition + 1,
            })
            return { score: VERY_HIGH_SCORE, ranges: matchedRanges }
          } else if (matchedRanges.length === 0) {
            // Internal match
            matchedRanges.push({
              start: substringPosition,
              end: substringPosition + 1,
            })
          }
        }
        substringPosition = folded.indexOf(this.needle, substringPosition + 1)
      }
      if (matchedRanges.length === 0) {
        return { score: 0, ranges: [] }
      } else {
        // First internal match
        console.assert(matchedRanges.length === 1)
        const position = matchedRanges[matchedRanges.length - 1].start
        return {
          score: Math.min(1 - position / folded.length, 0.01),
          ranges: matchedRanges,
        }
      }
    }

    // Filter with consecutive match
    let score = consecutiveMatchWithGaps(this.needle, folded, matchedRanges)
    if (score === 0) {
      matchedRanges.length = 0
      return { score: 0, ranges: [] }
    } else if (n > MAX_HAYSTACK || m > MAX_NEEDLE) {
      return { score, ranges: matchedRanges }
    }

    matchedRanges.length = 0
    return this.matrixMatch(this.needle, folded, matchedRanges)
  }

  private matrixMatch(
    needleString: string,
    haystackString: string,
    matchedRanges: Range[],
  ): { score: number; ranges: Range[] } {
    const MATCH_SCORE = 16
    const BOUNDARY_BONUS = 8
    const CONSECUTIVE_BONUS = 4
    const INITIAL_BONUS = BOUNDARY_BONUS * 2
    const GAP_START = 3
    const GAP_EXTENSION = 1

    const m = lengthInCodePoints(needleString)
    const n = lengthInCodePoints(haystackString)

    console.assert(m <= MAX_NEEDLE)
    console.assert(n <= MAX_HAYSTACK)

    this.scoreMatrix = new Int32Array(m * n)
    this.consecutiveMatrix = new Int32Array(m * n)
    this.wordBoundaries = new Array(n).fill(false)
    this.codepointToOffset = new Uint32Array(n)

    const needleChars = [...needleString]
    const haystackChars = [...haystackString]

    // Fill in first row and word boundaries
    let inGap = false
    const needleCodePoint = needleChars[0]
    this.wordBoundaries[0] = true
    let offset = 0

    for (let i = 0; i < haystackChars.length; i++) {
      const haystackCodePoint = haystackChars[i]
      this.codepointToOffset[i] = offset

      if (i < n - 1) {
        this.wordBoundaries[i + 1] = /\s/.test(haystackCodePoint)
      }

      const bonus = this.wordBoundaries[i] ? INITIAL_BONUS : 0
      if (needleCodePoint === haystackCodePoint) {
        this.consecutiveMatrix[i] = 1
        this.scoreMatrix[i] = MATCH_SCORE + bonus
        inGap = false
      } else {
        const penalty = inGap ? GAP_EXTENSION : GAP_START
        const leftScore = i > 0 ? this.scoreMatrix[i - 1] : 0
        this.scoreMatrix[i] = Math.max(leftScore - penalty, 0)
        inGap = true
      }
      offset += haystackCodePoint.length
    }

    // Fill in rows 1 through m - 1
    for (let j = 1; j < needleChars.length; j++) {
      inGap = false
      for (let i = 0; i < haystackChars.length; i++) {
        const idx = i + j * n

        if (i < j) {
          // Since all of needle must match, by the time we've gotten to the jth
          // character of needle, at least j characters of haystack have been consumed
          continue
        }

        // If we choose left_score, we're either creating or extending a gap
        let leftScore = i > 0 ? this.scoreMatrix[idx - 1] : 0
        const penalty = inGap ? GAP_EXTENSION : GAP_START
        leftScore -= penalty

        // If we choose diagonal_score, we're extending a match
        let diagonalScore = 0
        let consecutive = 0

        if (needleChars[j] === haystackChars[i]) {
          console.assert(j > 0)
          console.assert(i >= j)
          const diagonalIndex = idx - n - 1
          diagonalScore = this.scoreMatrix[diagonalIndex] + MATCH_SCORE

          if (this.wordBoundaries[j]) {
            diagonalScore += BOUNDARY_BONUS
            consecutive = 1
          } else {
            consecutive = this.consecutiveMatrix[idx - n - 1] + 1
            if (consecutive > 1) {
              const runStart = i - consecutive + 1
              diagonalScore += this.wordBoundaries[runStart]
                ? BOUNDARY_BONUS
                : CONSECUTIVE_BONUS
            }
          }
        }

        inGap = leftScore > diagonalScore
        this.consecutiveMatrix[idx] = inGap ? 0 : consecutive
        this.scoreMatrix[idx] = Math.max(0, Math.max(leftScore, diagonalScore))
      }
    }

    const rawScore = scoreForMatrix(
      this.scoreMatrix,
      n,
      m,
      this.codepointToOffset,
      matchedRanges,
    )
    const maxPossibleScore =
      INITIAL_BONUS + MATCH_SCORE + (BOUNDARY_BONUS + MATCH_SCORE) * (m - 1)
    const SCORE_BIAS = 0.25
    const score = SCORE_BIAS + (rawScore / maxPossibleScore) * (1 - SCORE_BIAS)
    console.assert(score <= 1.0)

    // Make sure it scores below exact matches and prefixes
    return { score: score * VERY_HIGH_SCORE, ranges: matchedRanges }
  }
}
