/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// `normalizeScaleAndTranslation` and the resample/distance logic built on it
// must stay in sync with their counterparts in
// brave/components/gestures/core/common/custom/custom_gesture_geometry_util.cc,
// used by `CustomGestureRecognizer` for actual matching. `centerInUnitSquare`
// has no C++ counterpart; it's display-only, for the preview icon. This file
// is used only for a pre-save "this looks like an existing gesture" warning,
// not for actual gesture matching (which is always done in C++ at gesture
// time).
const CUSTOM_GESTURE_RESAMPLE_POINT_COUNT = 32
export const CUSTOM_GESTURE_SIMILARITY_WARNING_THRESHOLD = 0.12
const LIVE_DRAW_RESAMPLE_POINT_COUNT = 24

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

function resampleToCount(path: Point[], count: number): Point[] {
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

function normalizeScaleAndTranslation(path: Point[]): Point[] {
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

// A non-square bounding box's shorter axis has a scaled extent less than 1;
// splitting the leftover space evenly on both sides centers it in the unit
// square instead of leaving it anchored to the top-left corner. Kept
// separate from `normalizeScaleAndTranslation` itself, which the similarity
// warning also depends on: centering based on each draw's own (noisy,
// hand-drawn) bounding box would add per-draw variance to that distance
// metric, pushing genuine near-duplicates past the threshold.
function centerInUnitSquare(path: Point[]): Point[] {
  const maxX = Math.max(...path.map((point) => point.x))
  const maxY = Math.max(...path.map((point) => point.y))
  const offsetX = (1 - maxX) / 2
  const offsetY = (1 - maxY) / 2
  return path.map((point) => ({
    x: offsetX + point.x,
    y: offsetY + point.y,
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

// Used to warn before saving a new custom gesture that looks too similar to
// one already recorded. Returns the smallest average distance from `path`
// to any of `customGestures`, or null if there are none to compare against.
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

// Approximates a Catmull-Rom spline with cubic-bezier control points.
function catmullRomControlPoints(
  previousPoint: Point, segmentStart: Point, segmentEnd: Point,
  nextPoint: Point): { controlPoint1: Point, controlPoint2: Point } {
  // Standard tangent scale for approximating a uniform Catmull-Rom spline
  // with cubic-bezier control points.
  const CATMULL_ROM_TANGENT_SCALE = 6
  const controlPoint1 = {
    x: segmentStart.x +
      (segmentEnd.x - previousPoint.x) / CATMULL_ROM_TANGENT_SCALE,
    y: segmentStart.y +
      (segmentEnd.y - previousPoint.y) / CATMULL_ROM_TANGENT_SCALE,
  }
  const controlPoint2 = {
    x: segmentEnd.x -
      (nextPoint.x - segmentStart.x) / CATMULL_ROM_TANGENT_SCALE,
    y: segmentEnd.y -
      (nextPoint.y - segmentStart.y) / CATMULL_ROM_TANGENT_SCALE,
  }
  return { controlPoint1, controlPoint2 }
}

// Raw captured points would render as a jagged polyline with sharp corners,
// which looks poor in the preview shown to the user.
export function smoothedSvgPath(path: Point[]): string {
  if (path.length < 2) {
    return ''
  }
  if (path.length === 2) {
    return `M ${path[0].x} ${path[0].y} L ${path[1].x} ${path[1].y}`
  }

  let d = `M ${path[0].x} ${path[0].y}`
  for (let i = 0; i < path.length - 1; ++i) {
    const previousPoint = path[Math.max(i - 1, 0)]
    const segmentStart = path[i]
    const segmentEnd = path[i + 1]
    const nextPoint = path[Math.min(i + 2, path.length - 1)]
    const { controlPoint1, controlPoint2 } = catmullRomControlPoints(
      previousPoint, segmentStart, segmentEnd, nextPoint)
    d += ` C ${controlPoint1.x} ${controlPoint1.y}, ` +
      `${controlPoint2.x} ${controlPoint2.y}, ${segmentEnd.x} ${segmentEnd.y}`
  }
  return d
}

// Preserves aspect ratio, so a tall or wide shape doesn't get stretched to
// fill the square preview.
export function fitToViewBox(
    path: Point[], size: number, padding: number): Point[] {
  // Resample before normalizing, so the preview's curve doesn't inherit any
  // jitter in the originally-recorded raw points.
  const resampled = resampleToCount(path, LIVE_DRAW_RESAMPLE_POINT_COUNT)
  const normalized = centerInUnitSquare(normalizeScaleAndTranslation(resampled))
  const available = size - padding * 2
  return normalized.map((point) => ({
    x: padding + point.x * available,
    y: padding + point.y * available,
  }))
}
