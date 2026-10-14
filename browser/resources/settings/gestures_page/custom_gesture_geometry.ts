/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Must stay in sync with its C++ counterpart, used for actual gesture
// matching; this file only drives a pre-save similarity warning.
const CUSTOM_GESTURE_RESAMPLE_POINT_COUNT = 128
export const CUSTOM_GESTURE_MAX_MATCH_DISTANCE = 0.14

// Deliberately stricter than `CUSTOM_GESTURE_MAX_MATCH_DISTANCE`. This only
// drives the pre-save similarity warning, with no C++ counterpart.
export const CUSTOM_GESTURE_SHAPE_CONFLICT_MAX_DISTANCE = 0.1

export interface Point {
  x: number
  y: number
}

function pathLength(gesture_path: Point[]): number {
  let length = 0
  for (let i = 1; i < gesture_path.length; ++i) {
    length += Math.hypot(gesture_path[i].x - gesture_path[i - 1].x, gesture_path[i].y - gesture_path[i - 1].y)
  }
  return length
}

export function resampleToCount(gesture_path: Point[], count: number): Point[] {
  const totalLength = pathLength(gesture_path)
  if (totalLength === 0) {
    return new Array(count).fill(gesture_path[0])
  }

  const interval = totalLength / (count - 1)
  const resampled: Point[] = [gesture_path[0]]
  let accumulated = 0

  for (let i = 1; i < gesture_path.length && resampled.length < count; ++i) {
    let previous = gesture_path[i - 1]
    const current = gesture_path[i]
    let segmentLength =
      Math.hypot(current.x - previous.x, current.y - previous.y)

    while (accumulated + segmentLength >= interval &&
           resampled.length < count) {
      const remaining = interval - accumulated
      const ratio = segmentLength > 0 ? remaining / segmentLength : 0
      const newPoint = {
        x: previous.x + ratio * (current.x - previous.x),
        y: previous.y + ratio * (current.y - previous.y),
      }
      resampled.push(newPoint)
      segmentLength -= remaining
      accumulated = 0
      // Advances from the newly interpolated point, not the original
      // segment's start, so a single long original segment resamples into
      // several evenly-spaced points rather than repeatedly interpolating
      // from the same fixed start (see `ResampleGesturePathAtInterval`'s
      // identical fix on the C++ side).
      previous = newPoint
    }
    accumulated += segmentLength
  }

  while (resampled.length < count) {
    resampled.push(gesture_path[gesture_path.length - 1])
  }
  return resampled
}

export function normalizeScaleAndTranslation(gesture_path: Point[]): Point[] {
  const minX = Math.min(...gesture_path.map((point) => point.x))
  const minY = Math.min(...gesture_path.map((point) => point.y))
  const maxX = Math.max(...gesture_path.map((point) => point.x))
  const maxY = Math.max(...gesture_path.map((point) => point.y))
  const scale = Math.max(maxX - minX, maxY - minY)

  if (scale === 0) {
    return gesture_path.map(() => ({ x: 0, y: 0 }))
  }
  return gesture_path.map((point) => ({
    x: (point.x - minX) / scale,
    y: (point.y - minY) / scale,
  }))
}

function resampleAndNormalizePath(gesture_path: Point[], count: number): Point[] {
  return normalizeScaleAndTranslation(resampleToCount(gesture_path, count))
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

// Resamples and normalizes both `gesture_path` and `candidate` the same way real
// matching does, then returns their average per-point distance. Exposed so
// a candidate that isn't itself a recorded custom gesture -- e.g. a
// directional stroke's idealized template -- can be compared against a
// drawn gesture_path on the same scale.
export function distanceToShape(gesture_path: Point[], candidate: Point[]): number {
  const normalizedPath =
    resampleAndNormalizePath(gesture_path, CUSTOM_GESTURE_RESAMPLE_POINT_COUNT)
  const normalizedCandidate =
    resampleAndNormalizePath(candidate, CUSTOM_GESTURE_RESAMPLE_POINT_COUNT)
  return averagePerPointDistance(normalizedPath, normalizedCandidate)
}

// The smallest average distance from `gesture_path` to any of `customGestures`,
// or null if there are none to compare against.
export function closestCustomGestureDistance(
  gesture_path: Point[],
  customGestures: Array<{ points: Point[] }>): number | null {
  if (customGestures.length === 0) {
    return null
  }
  let smallestAverageDistance = Infinity
  for (const customGesture of customGestures) {
    if (customGesture.points.length < 2) {
      continue
    }
    smallestAverageDistance = Math.min(
      smallestAverageDistance, distanceToShape(gesture_path, customGesture.points))
  }
  return smallestAverageDistance
}
