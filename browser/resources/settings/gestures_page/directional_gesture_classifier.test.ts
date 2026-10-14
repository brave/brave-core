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
    const path = [{x: 0, y: 0}, {x: 100, y: 0}]

    expect(classifyDirectionalStrokeKey(path, 30, 0.9, 4)).toBe('→')
  })

  test('should classify a clean turn as two separate strokes', () => {
    const path = [{x: 0, y: 0}, {x: 100, y: 0}, {x: 100, y: 100}]

    expect(classifyDirectionalStrokeKey(path, 30, 0.9, 4)).toBe('→↓')
  })

  test('should not commit a stroke for a continuously curving path', () => {
    // A hand-drawn oval/circle: no single direction ever sustains on its
    // own before the curve has already moved on to the next octant.
    const path: Array<{x: number, y: number}> = []
    const pointCount = 16
    const radius = 20
    for (let i = 1; i <= pointCount; ++i) {
      const angle = 2 * Math.PI * i / pointCount
      path.push({
        x: 100 + radius * Math.cos(angle),
        y: 100 + radius * Math.sin(angle),
      })
    }

    expect(classifyDirectionalStrokeKey(path, 30, 0.9, 4)).toBe('')
  })

  test('should not treat a coincidental loop-back as continuing the stroke', () => {
    // After a rightward stroke commits, a loop that happens to swing back
    // through the right octant relative to the stroke's own origin must not
    // be accepted as "continuing" it just because it lands in the same
    // direction -- the loop's own arc length must still count against
    // straightness, same as it would for any other divergence.
    const path = [
      {x: 0, y: 0}, {x: 30, y: 0}, {x: 60, y: 0},
      {x: 60, y: 50}, {x: 110, y: 50}, {x: 90, y: 0},
      {x: 90, y: 1000}, {x: 90, y: 1500}, {x: 90, y: 2000},
    ]

    expect(classifyDirectionalStrokeKey(path, 30, 0.9, 4)).toBe('→↓')
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
