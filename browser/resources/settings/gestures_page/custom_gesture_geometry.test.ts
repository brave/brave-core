/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  closestCustomGestureDistance,
  distanceToShape,
  normalizeScaleAndTranslation,
  resampleToCount,
} from './custom_gesture_geometry'

describe('resampleToCount', () => {
  test('should return exactly the requested point count', () => {
    const path = [{x: 0, y: 0}, {x: 10, y: 0}]

    expect(resampleToCount(path, 5)).toHaveLength(5)
  })

  test('should evenly space points resampled from one long segment', () => {
    // A 2-point input resampled to many more output points requires
    // repeated sub-division of that single original segment. Each
    // extracted point must advance from the previous one, not re-interpolate
    // from the segment's fixed original start every time, or points bunch
    // up near the start instead of spacing evenly (see the identical fix on
    // the C++ side, `ResampleToCount`).
    const path = [{x: 0, y: 0}, {x: 10, y: 0}]

    const resampled = resampleToCount(path, 5)

    expect(resampled.map((point) => point.x)).toEqual([0, 2.5, 5, 7.5, 10])
  })

  test('should collapse a zero-length path to its single point', () => {
    const path = [{x: 5, y: 5}, {x: 5, y: 5}]

    expect(resampleToCount(path, 4)).toEqual([
      {x: 5, y: 5}, {x: 5, y: 5}, {x: 5, y: 5}, {x: 5, y: 5},
    ])
  })
})

describe('normalizeScaleAndTranslation', () => {
  test('should translate and scale to fit within [0, 1]', () => {
    const path = [{x: 10, y: 20}, {x: 20, y: 20}, {x: 20, y: 40}]

    expect(normalizeScaleAndTranslation(path)).toEqual([
      {x: 0, y: 0}, {x: 0.5, y: 0}, {x: 0.5, y: 1},
    ])
  })

  test('should collapse a zero-length path to the origin', () => {
    const path = [{x: 5, y: 5}, {x: 5, y: 5}]

    expect(normalizeScaleAndTranslation(path)).toEqual([
      {x: 0, y: 0}, {x: 0, y: 0},
    ])
  })
})

describe('distanceToShape', () => {
  test('should return zero for a shape compared against itself', () => {
    const path = [
      {x: 0, y: 0}, {x: 50, y: 10}, {x: 100, y: 0}, {x: 150, y: 20},
    ]

    expect(distanceToShape(path, path)).toBeCloseTo(0)
  })

  test('should return a large distance for dissimilar shapes', () => {
    const straightLine = [{x: 0, y: 0}, {x: 100, y: 0}]
    const vShape = [{x: 0, y: 0}, {x: 50, y: 100}, {x: 100, y: 0}]

    expect(distanceToShape(straightLine, vShape)).toBeGreaterThan(0.14)
  })
})

describe('closestCustomGestureDistance', () => {
  test('should return null when there are no custom gestures to compare', () => {
    const path = [{x: 0, y: 0}, {x: 100, y: 0}]

    expect(closestCustomGestureDistance(path, [])).toBeNull()
  })

  test('should return the smallest distance among several custom gestures', () => {
    const path = [{x: 0, y: 0}, {x: 100, y: 0}]
    const closeMatch = {points: [{x: 0, y: 0}, {x: 100, y: 1}]}
    const farMatch = {points: [{x: 0, y: 0}, {x: 50, y: 100}, {x: 100, y: 0}]}

    const distance = closestCustomGestureDistance(
      path, [farMatch, closeMatch])

    expect(distance).not.toBeNull()
    expect(distance as number).toBeLessThan(0.05)
  })

  test('should skip custom gestures with fewer than two points', () => {
    const path = [{x: 0, y: 0}, {x: 100, y: 0}]
    const tooFewPoints = {points: [{x: 0, y: 0}]}

    expect(closestCustomGestureDistance(path, [tooFewPoints])).toBe(Infinity)
  })
})
