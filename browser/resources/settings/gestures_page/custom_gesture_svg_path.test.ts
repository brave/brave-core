/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {fitToViewBox, svgPathData} from './custom_gesture_svg_path'

describe('svgPathData', () => {
  test('should return empty for an empty path', () => {
    expect(svgPathData([])).toBe('')
  })

  test('should return empty for a single point', () => {
    expect(svgPathData([{x: 0, y: 0}])).toBe('')
  })

  test('should return a straight line for exactly two points', () => {
    expect(svgPathData([{x: 0, y: 0}, {x: 10, y: 20}]))
      .toBe('M 0 0 L 10 20')
  })

  test('should return a curve starting and ending at the path endpoints', () => {
    const path = [{x: 0, y: 0}, {x: 10, y: 0}, {x: 10, y: 10}]

    const pathData = svgPathData(path)

    expect(pathData.startsWith('M 0 0')).toBe(true)
    expect(pathData.endsWith('10 10')).toBe(true)
    // One cubic-bezier segment per pair of consecutive points.
    expect(pathData.match(/C /g)?.length).toBe(path.length - 1)
  })
})

describe('fitToViewBox', () => {
  test('should resample to a fixed point count regardless of input length', () => {
    const path = [{x: 0, y: 0}, {x: 10, y: 10}]

    expect(fitToViewBox(path, 100, 10).length).toBe(24)
  })

  test('should keep every point within the padded view box', () => {
    const path = [{x: 0, y: 0}, {x: 30, y: 5}, {x: 15, y: 40}]
    const size = 100
    const padding = 10

    const fitted = fitToViewBox(path, size, padding)

    for (const point of fitted) {
      expect(point.x).toBeGreaterThanOrEqual(padding)
      expect(point.x).toBeLessThanOrEqual(size - padding)
      expect(point.y).toBeGreaterThanOrEqual(padding)
      expect(point.y).toBeLessThanOrEqual(size - padding)
    }
  })

  test('should center a degenerate path with zero length at the middle of the view box', () => {
    const path = [{x: 5, y: 5}, {x: 5, y: 5}]
    const size = 100
    const padding = 10

    const fitted = fitToViewBox(path, size, padding)
    const center = padding + (size - padding * 2) / 2

    for (const point of fitted) {
      expect(point.x).toBeCloseTo(center)
      expect(point.y).toBeCloseTo(center)
    }
  })
})
