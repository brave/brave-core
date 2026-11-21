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

// Must stay in sync with its C++ counterpart.
const CUSTOM_GESTURE_CLOSED_PATH_GAP_RATIO = 0.2

export interface Point {
  x: number
  y: number
}

// Rejects jitter too close to the last accepted point, and further points
// once `maxPointCount` is reached, so a drawn gesture can't grow unbounded.
export function shouldAcceptDrawnPoint(
  points: Point[], point: Point, minDistancePixels: number,
  maxPointCount: number): boolean {
  if (points.length >= maxPointCount) {
    return false
  }
  const lastPoint = points[points.length - 1]
  return Math.hypot(point.x - lastPoint.x, point.y - lastPoint.y) >=
    minDistancePixels
}

function pathLength(gesture_path: Point[]): number {
  let totalLength = 0
  for (let i = 1; i < gesture_path.length; ++i) {
    totalLength += Math.hypot(gesture_path[i].x - gesture_path[i - 1].x, gesture_path[i].y - gesture_path[i - 1].y)
  }
  return totalLength
}

export function resampleToCount(gesture_path: Point[], count: number): Point[] {
  const totalLength = pathLength(gesture_path)
  if (totalLength === 0) {
    return new Array(count).fill(gesture_path[0])
  }

  const interval = totalLength / (count - 1)
  const resampledPath: Point[] = [gesture_path[0]]
  let accumulatedDistance = 0

  for (let i = 1; i < gesture_path.length && resampledPath.length < count;
      ++i) {
    let previousPoint = gesture_path[i - 1]
    const currentPoint = gesture_path[i]
    let segmentLength =
      Math.hypot(currentPoint.x - previousPoint.x, currentPoint.y - previousPoint.y)

    while (accumulatedDistance + segmentLength >= interval &&
           resampledPath.length < count) {
      const remainingIntervalDistance = interval - accumulatedDistance
      const ratio =
        segmentLength > 0 ? remainingIntervalDistance / segmentLength : 0
      const newPoint = {
        x: previousPoint.x + ratio * (currentPoint.x - previousPoint.x),
        y: previousPoint.y + ratio * (currentPoint.y - previousPoint.y),
      }
      resampledPath.push(newPoint)
      segmentLength -= remainingIntervalDistance
      accumulatedDistance = 0
      // Advances from the new point, not the segment's start, matching
      // `AppendResampledPoints` in gesture_path_resample_util.cc.
      previousPoint = newPoint
    }
    accumulatedDistance += segmentLength
  }

  while (resampledPath.length < count) {
    resampledPath.push(gesture_path[gesture_path.length - 1])
  }
  return resampledPath
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
  let totalDistance = 0
  for (let i = 0; i < firstPath.length; ++i) {
    totalDistance += Math.hypot(
      firstPath[i].x - secondPath[i].x, firstPath[i].y - secondPath[i].y)
  }
  return totalDistance / firstPath.length
}

// A closed shape's (e.g. a circle) start point is arbitrary, so `candidate`
// is compared at every rotation of its points, and winding direction, and
// the closest one is used; matches `ForEachClosedGesturePathRotation`.
function rotatePath(gesture_path: Point[], offset: number): Point[] {
  return gesture_path.map(
    (_, i) => gesture_path[(offset + i) % gesture_path.length])
}

function closedPathRotations(candidate: Point[]): Point[][] {
  const reversedCandidate = [...candidate].reverse()
  const rotations: Point[][] = []
  for (const path of [candidate, reversedCandidate]) {
    for (let offset = 0; offset < path.length; ++offset) {
      rotations.push(rotatePath(path, offset))
    }
  }
  return rotations
}

function bestCandidateDistance(
  gesture_path: Point[], candidate: Point[],
  candidateIsClosed: boolean): number {
  if (!candidateIsClosed) {
    return averagePerPointDistance(gesture_path, candidate)
  }
  return Math.min(...closedPathRotations(candidate).map(
    (rotatedCandidate) =>
      averagePerPointDistance(gesture_path, rotatedCandidate)))
}

// Resamples and normalizes both paths the same way real matching does, so a
// non-saved candidate (e.g. a directional template) still compares fairly.
export function distanceToShape(
  gesture_path: Point[], candidate: Point[],
  candidateIsClosed = false): number {
  const normalizedPath =
    resampleAndNormalizePath(gesture_path, CUSTOM_GESTURE_RESAMPLE_POINT_COUNT)
  const normalizedCandidate =
    resampleAndNormalizePath(candidate, CUSTOM_GESTURE_RESAMPLE_POINT_COUNT)
  return bestCandidateDistance(
    normalizedPath, normalizedCandidate, candidateIsClosed)
}

// True if `gesture_path`'s start and end are close relative to its own
// length (e.g. a circle), meaning its start point is arbitrary.
export function isClosedPath(gesture_path: Point[]): boolean {
  const length = pathLength(gesture_path)
  if (length === 0) {
    return false
  }
  const firstPoint = gesture_path[0]
  const lastPoint = gesture_path[gesture_path.length - 1]
  const endToStartDistance =
    Math.hypot(lastPoint.x - firstPoint.x, lastPoint.y - firstPoint.y)
  return endToStartDistance / length < CUSTOM_GESTURE_CLOSED_PATH_GAP_RATIO
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
      smallestAverageDistance,
      distanceToShape(
        gesture_path, customGesture.points,
        isClosedPath(customGesture.points)))
  }
  return smallestAverageDistance
}
