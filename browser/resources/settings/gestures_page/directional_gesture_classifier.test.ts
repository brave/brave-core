/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  makeCanonicalStrokeTemplate,
  classifyDirectionalStrokeKey,
} from './directional_gesture_classifier'

describe('classifyDirectionalStrokeKey', () => {
  test('should classify a straight rightward swipe as a single right stroke', () => {
    const gesturePath = [{ x: 0, y: 0 }, { x: 100, y: 0 }]

    expect(classifyDirectionalStrokeKey(gesturePath, 30, 0.9, 4)).toBe('→')
  })

  test('should classify a clean turn as two separate strokes', () => {
    const gesturePath = [{ x: 0, y: 0 }, { x: 100, y: 0 }, { x: 100, y: 100 }]

    expect(classifyDirectionalStrokeKey(gesturePath, 30, 0.9, 4)).toBe('→↓')
  })

  test('should not commit a stroke for a continuously curving gesture path', () => {
    // A hand-drawn oval/circle: no single direction ever sustains on its
    // own before the curve has already moved on to the next octant.
    const gesturePath: Array<{ x: number, y: number }> = []
    const pointCount = 16
    const radius = 20
    for (let i = 1; i <= pointCount; ++i) {
      const angle = 2 * Math.PI * i / pointCount
      gesturePath.push({
        x: 100 + radius * Math.cos(angle),
        y: 100 + radius * Math.sin(angle),
      })
    }

    expect(classifyDirectionalStrokeKey(gesturePath, 30, 0.9, 4)).toBe('')
  })

  // The next three tests share their exact points with same-named tests in
  // directional_gesture_stroke_classifier_util_unittest.cc; keep both in sync.

  // Swinging back through the same octant must still count the loop's own
  // arc length against straightness, not be accepted as a continuation.
  test('should not extend a stroke for an incidental loop-back', () => {
    const gesturePath = [
      {x: 0, y: 0}, {x: 30, y: 0}, {x: 60, y: 0},
      {x: 60, y: 50}, {x: 110, y: 50}, {x: 90, y: 0},
      {x: 90, y: 1000}, {x: 90, y: 1500}, {x: 90, y: 2000},
    ]

    expect(classifyDirectionalStrokeKey(gesturePath, 30, 0.9, 4)).toBe('→↓')
  })

  // A 90-degree turn after a committed stroke must commit as its own
  // stroke, not get absorbed, even with a brief overshoot past the corner.
  test('should recognize a second stroke after a sustained direction change',
    () => {
      const gesturePath = [
        {x: 0, y: 0}, {x: 10, y: 0}, {x: 20, y: 0}, {x: 30, y: 0},
        {x: 40, y: 0}, {x: 50, y: 0}, {x: 60, y: 0}, {x: 70, y: 0},
        {x: 80, y: 0}, {x: 90, y: 0}, {x: 100, y: 0}, {x: 100, y: 10},
        {x: 100, y: 20}, {x: 100, y: 30}, {x: 100, y: 40}, {x: 100, y: 50},
        {x: 100, y: 60},
      ]

      expect(classifyDirectionalStrokeKey(gesturePath, 30, 0.9, 4)).toBe('→↓')
    })

  // A gesture_path that keeps changing candidate direction must not let a
  // short, locally-straight tail "launder" the curve that preceded it.
  test('should not recognize a wandering curve as a stroke', () => {
    const gesturePath = [
      {x: 0, y: 0}, {x: 10, y: 2}, {x: 18, y: 8}, {x: 22, y: 18},
      {x: 20, y: 28}, {x: 12, y: 35}, {x: 2, y: 36}, {x: -6, y: 30},
      {x: -8, y: 20}, {x: -4, y: 10}, {x: 10, y: 8}, {x: 25, y: 8},
      {x: 40, y: 8},
    ]

    expect(classifyDirectionalStrokeKey(gesturePath, 30, 0.9, 4)).toBe('')
  })
})

describe('makeCanonicalStrokeTemplate', () => {
  test('should chain a unit segment per stroke', () => {
    expect(makeCanonicalStrokeTemplate('→↓')).toEqual([
      {x: 0, y: 0}, {x: 1, y: 0}, {x: 1, y: 1},
    ])
  })

  test('should return just the origin for an empty key', () => {
    expect(makeCanonicalStrokeTemplate('')).toEqual([{x: 0, y: 0}])
  })
})
