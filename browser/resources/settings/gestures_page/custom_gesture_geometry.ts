/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Must stay in sync with its C++ counterpart, used for actual gesture
// matching; this file only drives a pre-save similarity warning.
const CUSTOM_GESTURE_RESAMPLE_POINT_COUNT = 32
export const CUSTOM_GESTURE_SIMILARITY_WARNING_THRESHOLD = 0.12

export interface Point {
  x: number
  y: number
}

function pathLength(path: Point[]): number {
  let length = 0
  for (let i = 1; i < path.length; ++i) {
    length += Math.hypot(path[i].x - path[i - 1].x, path[i].y - path[i - 1].y)
  }
  return length
}

export function resampleToCount(path: Point[], count: number): Point[] {
  const totalLength = pathLength(path)
  if (totalLength === 0) {
    return new Array(count).fill(path[0])
  }

  const interval = totalLength / (count - 1)
  const resampled: Point[] = [path[0]]
  let accumulated = 0

  for (let i = 1; i < path.length && resampled.length < count; ++i) {
    const previous = path[i - 1]
    const current = path[i]
    let segmentLength =
      Math.hypot(current.x - previous.x, current.y - previous.y)

    while (accumulated + segmentLength >= interval &&
           resampled.length < count) {
      const remaining = interval - accumulated
      const ratio = segmentLength > 0 ? remaining / segmentLength : 0
      resampled.push({
        x: previous.x + ratio * (current.x - previous.x),
        y: previous.y + ratio * (current.y - previous.y),
      })
      segmentLength -= remaining
      accumulated = 0
    }
    accumulated += segmentLength
  }

  while (resampled.length < count) {
    resampled.push(path[path.length - 1])
  }
  return resampled
}

export function normalizeScaleAndTranslation(path: Point[]): Point[] {
  const minX = Math.min(...path.map((point) => point.x))
  const minY = Math.min(...path.map((point) => point.y))
  const maxX = Math.max(...path.map((point) => point.x))
  const maxY = Math.max(...path.map((point) => point.y))
  const scale = Math.max(maxX - minX, maxY - minY)

  if (scale === 0) {
    return path.map(() => ({ x: 0, y: 0 }))
  }
  return path.map((point) => ({
    x: (point.x - minX) / scale,
    y: (point.y - minY) / scale,
  }))
}

function resampleAndNormalizePath(path: Point[], count: number): Point[] {
  return normalizeScaleAndTranslation(resampleToCount(path, count))
}

function averagePerPointDistance(firstPath: Point[], secondPath: Point[]):
  number {
  let total = 0
  for (let i = 0; i < firstPath.length; ++i) {
    total += Math.hypot(
      firstPath[i].x - secondPath[i].x, firstPath[i].y - secondPath[i].y)
  }
  return total / firstPath.length
}

// The smallest average distance from `path` to any of `customGestures`,
// or null if there are none to compare against.
export function closestCustomGestureDistance(
  path: Point[],
  customGestures: Array<{ points: Point[] }>): number | null {
  if (customGestures.length === 0) {
    return null
  }
  const normalizedPath =
    resampleAndNormalizePath(path, CUSTOM_GESTURE_RESAMPLE_POINT_COUNT)
  let smallestAverageDistance = Infinity
  for (const customGesture of customGestures) {
    if (customGesture.points.length < 2) {
      continue
    }
    const normalizedCustomGesture = resampleAndNormalizePath(
      customGesture.points, CUSTOM_GESTURE_RESAMPLE_POINT_COUNT)
    smallestAverageDistance = Math.min(
      smallestAverageDistance,
      averagePerPointDistance(normalizedPath, normalizedCustomGesture))
  }
  return smallestAverageDistance
}
