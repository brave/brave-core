/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  closestCustomGestureDistance,
  distanceToShape,
  isClosedPath,
  normalizeScaleAndTranslation,
  resampleToCount,
  shouldAcceptDrawnPoint,
} from './custom_gesture_geometry'

// Two independently-drawn squares, same shape, each starting at a
// different corner and closing near its own start point, not the other's;
// used below to test closed-shape rotation search.
const SQUARE_STARTING_AT_TOP_LEFT =
  [{x: 0, y: 0}, {x: 10, y: 0}, {x: 10, y: 10}, {x: 0, y: 10}, {x: 1, y: 1}]
const SQUARE_STARTING_AT_BOTTOM_RIGHT =
  [{x: 10, y: 10}, {x: 0, y: 10}, {x: 0, y: 0}, {x: 10, y: 0}, {x: 9, y: 9}]

describe('shouldAcceptDrawnPoint', () => {
  test('should accept a point far enough from the last one', () => {
    const points = [{x: 0, y: 0}]

    expect(shouldAcceptDrawnPoint(points, {x: 10, y: 0}, 3, 512)).toBe(true)
  })

  test('should reject jitter too close to the last accepted point', () => {
    const points = [{x: 0, y: 0}]

    expect(shouldAcceptDrawnPoint(points, {x: 1, y: 0}, 3, 512)).toBe(false)
  })

  test('should accept a point exactly at the minimum distance', () => {
    const points = [{x: 0, y: 0}]

    expect(shouldAcceptDrawnPoint(points, {x: 3, y: 0}, 3, 512)).toBe(true)
  })

  test('should reject once the maximum point count is reached', () => {
    const points = [{x: 0, y: 0}, {x: 10, y: 0}]

    expect(shouldAcceptDrawnPoint(points, {x: 20, y: 0}, 3, 2)).toBe(false)
  })
})

describe('resampleToCount', () => {
  test('should return exactly the requested point count', () => {
    const gesturePath = [{ x: 0, y: 0 }, { x: 10, y: 0 }]

    expect(resampleToCount(gesturePath, 5)).toHaveLength(5)
  })

  test('should evenly space points resampled from one long segment', () => {
    // Each extracted point must advance from the previous one, not
    // re-interpolate from the segment's fixed start, or points bunch up.
    const gesturePath = [{ x: 0, y: 0 }, { x: 10, y: 0 }]

    const resampled = resampleToCount(gesturePath, 5)

    expect(resampled.map((point) => point.x)).toEqual([0, 2.5, 5, 7.5, 10])
  })

  test('should collapse a zero-length gesture path to its single point', () => {
    const gesturePath = [{ x: 5, y: 5 }, { x: 5, y: 5 }]

    expect(resampleToCount(gesturePath, 4)).toEqual([
      {x: 5, y: 5}, {x: 5, y: 5}, {x: 5, y: 5}, {x: 5, y: 5},
    ])
  })
})

describe('normalizeScaleAndTranslation', () => {
  test('should translate and scale to fit within [0, 1]', () => {
    const gesturePath = [{ x: 10, y: 20 }, { x: 20, y: 20 }, { x: 20, y: 40 }]

    expect(normalizeScaleAndTranslation(gesturePath)).toEqual([
      {x: 0, y: 0}, {x: 0.5, y: 0}, {x: 0.5, y: 1},
    ])
  })

  test('should collapse a zero-length gesture path to the origin', () => {
    const gesturePath = [{ x: 5, y: 5 }, { x: 5, y: 5 }]

    expect(normalizeScaleAndTranslation(gesturePath)).toEqual([
      {x: 0, y: 0}, {x: 0, y: 0},
    ])
  })
})

describe('isClosedPath', () => {
  test('should return false for a zero-length path', () => {
    expect(isClosedPath([{x: 5, y: 5}, {x: 5, y: 5}])).toBe(false)
  })

  test('should return false for an open path', () => {
    expect(isClosedPath([{x: 0, y: 0}, {x: 100, y: 0}])).toBe(false)
  })

  test('should return true for a path whose end returns near its start',
    () => {
      expect(isClosedPath(SQUARE_STARTING_AT_TOP_LEFT)).toBe(true)
    })
})

describe('distanceToShape', () => {
  test('should return zero for a shape compared against itself', () => {
    const gesturePath = [
      {x: 0, y: 0}, {x: 50, y: 10}, {x: 100, y: 0}, {x: 150, y: 20},
    ]

    expect(distanceToShape(gesturePath, gesturePath)).toBeCloseTo(0)
  })

  test('should return a large distance for dissimilar shapes', () => {
    const straightLine = [{x: 0, y: 0}, {x: 100, y: 0}]
    const vShape = [{x: 0, y: 0}, {x: 50, y: 100}, {x: 100, y: 0}]

    expect(distanceToShape(straightLine, vShape)).toBeGreaterThan(0.14)
  })

  test('should return a large distance for the same closed shape drawn ' +
    'from a different corner when the candidate is not marked closed',
    () => {
      expect(distanceToShape(
        SQUARE_STARTING_AT_TOP_LEFT, SQUARE_STARTING_AT_BOTTOM_RIGHT))
        .toBeGreaterThan(0.14)
    })

  test('should return a small distance for the same closed shape drawn ' +
    'from a different corner when the candidate is marked closed', () => {
    expect(distanceToShape(
      SQUARE_STARTING_AT_TOP_LEFT, SQUARE_STARTING_AT_BOTTOM_RIGHT,
      /*candidateIsClosed=*/true))
      .toBeLessThan(0.05)
  })
})

describe('closestCustomGestureDistance', () => {
  test('should return null when there are no custom gestures to compare', () => {
    const gesturePath = [{ x: 0, y: 0 }, { x: 100, y: 0 }]

    expect(closestCustomGestureDistance(gesturePath, [])).toBeNull()
  })

  test('should return the smallest distance among several custom gestures', () => {
    const gesturePath = [{ x: 0, y: 0 }, { x: 100, y: 0 }]
    const closeMatch = {points: [{x: 0, y: 0}, {x: 100, y: 1}]}
    const farMatch = {points: [{x: 0, y: 0}, {x: 50, y: 100}, {x: 100, y: 0}]}

    const distance = closestCustomGestureDistance(
      gesturePath, [farMatch, closeMatch])

    expect(distance).not.toBeNull()
    expect(distance as number).toBeLessThan(0.05)
  })

  test('should skip custom gestures with fewer than two points', () => {
    const gesturePath = [{ x: 0, y: 0 }, { x: 100, y: 0 }]
    const tooFewPoints = {points: [{x: 0, y: 0}]}

    expect(closestCustomGestureDistance(gesturePath, [tooFewPoints])).toBe(Infinity)
  })

  test('should match a saved closed shape drawn from a different corner ' +
    'of the same shape', () => {
    const savedSquare = {points: SQUARE_STARTING_AT_BOTTOM_RIGHT}

    const distance = closestCustomGestureDistance(
      SQUARE_STARTING_AT_TOP_LEFT, [savedSquare])

    expect(distance).not.toBeNull()
    expect(distance as number).toBeLessThan(0.05)
  })
})
