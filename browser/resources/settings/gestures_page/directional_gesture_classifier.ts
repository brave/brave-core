/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {Point} from './custom_gesture_geometry.js'

// Mirrors `ResampleAtInterval`/`ClassifyDirectionalStrokes` in
// brave/components/gestures/core/common/directional/directional_gesture_util.cc,
// which `GestureDrawSession::ClassifyResampledDirectionalStrokes` runs
// against the raw traced path at gesture-finish time. Used only for a
// pre-save "this looks like a mapped directional gesture, so it would never
// actually run" warning; actual directional matching is always done in C++
// at gesture time.

// Clockwise angular order starting from "right" (screen angle 0), matching
// `kDirectionsByAngle` in directional_gesture_util.cc.
const DIRECTIONS_BY_ANGLE = ['→', '↘', '↓', '↙', '←', '↖', '↑', '↗']

function maybeMovementDirection(
    origin: Point, location: Point, minStrokePixels: number): string|null {
  const dx = location.x - origin.x
  const dy = location.y - origin.y
  if (Math.hypot(dx, dy) < minStrokePixels) {
    return null
  }
  let degrees = Math.atan2(dy, dx) * 180 / Math.PI
  if (degrees < 0) {
    degrees += 360
  }
  const octant = Math.round(degrees / 45) % DIRECTIONS_BY_ANGLE.length
  return DIRECTIONS_BY_ANGLE[octant]
}

// Mirrors `ResampleAtInterval`: resamples `path` to points evenly spaced
// `intervalPixels` apart by arc length, so classification doesn't depend on
// how unevenly the original points happened to be sampled (e.g. a brief
// slowdown mid-drag producing two unusually close consecutive points, which
// could otherwise fail a sustain check that a steadier draw of the exact
// same shape would have passed).
function resampleAtInterval(path: Point[], intervalPixels: number): Point[] {
  if (path.length === 0) {
    return []
  }
  const resampled: Point[] = [path[0]]
  if (path.length < 2 || intervalPixels <= 0) {
    return resampled
  }

  // Carries over leftover distance from one original segment to the next, so
  // intervals stay consistent across the original points' own boundaries,
  // not just within each one.
  let accumulated = 0
  let lastPoint = path[0]
  for (let i = 1; i < path.length; ++i) {
    const current = path[i]
    let segmentLength =
        Math.hypot(current.x - lastPoint.x, current.y - lastPoint.y)

    while (accumulated + segmentLength >= intervalPixels) {
      const remaining = intervalPixels - accumulated
      const ratio = segmentLength > 0 ? remaining / segmentLength : 0
      const newPoint = {
        x: lastPoint.x + ratio * (current.x - lastPoint.x),
        y: lastPoint.y + ratio * (current.y - lastPoint.y),
      }
      resampled.push(newPoint)
      segmentLength -= remaining
      // Advances from the newly interpolated point, not the original
      // segment's start, so a single long original segment resamples into
      // several evenly-spaced points rather than repeatedly interpolating
      // from the same fixed start.
      lastPoint = newPoint
      accumulated = 0
    }
    accumulated += segmentLength
    lastPoint = current
  }
  const last = resampled[resampled.length - 1]
  const end = path[path.length - 1]
  if (last.x !== end.x || last.y !== end.y) {
    resampled.push(end)
  }
  return resampled
}

// Mirrors `ClassifyDirectionalStrokes`: classifies `path` into a sequence of
// directional strokes, capped at `maxStrokes`. `origin` is always either the
// start of the current committed stroke or the start of the current pending
// candidate -- it advances every time the candidate direction changes, not
// just on commit. Measuring from a stale anchor left over from several
// direction-changes ago reads a slowly-drifting aggregate bearing on a
// curved path, which can disagree with the real local tangent for the
// entire rest of the path and prevent any further stroke from ever
// committing -- e.g. a hand-drawn oval collapsing to a single stroke.
function classifyDirectionalStrokes(
    path: Point[], minStrokePixels: number, maxStrokes: number): string[] {
  const strokes: string[] = []
  if (path.length === 0) {
    return strokes
  }

  let origin = path[0]
  let pendingDirection: string|null = null

  for (let i = 1; i < path.length && strokes.length < maxStrokes; ++i) {
    const location = path[i]
    const direction = maybeMovementDirection(origin, location, minStrokePixels)
    if (direction === null) {
      continue
    }

    if (strokes.length > 0 && direction === strokes[strokes.length - 1]) {
      pendingDirection = null
      origin = location
      continue
    }

    if (pendingDirection !== direction) {
      pendingDirection = direction
      origin = location
      continue
    }

    strokes.push(direction)
    pendingDirection = null
    origin = location
  }

  return strokes
}

// Classifies `rawPoints` (in the same raw pixel units `minStrokePixels` is
// calibrated in, not normalized) into the stroke-key string
// `DirectionalGestureMapping` looks mappings up by, e.g. "←" or "↓→".
export function classifyDirectionalStrokeKey(
    rawPoints: Point[], minStrokePixels: number, maxStrokes: number):
    string {
  const resampled = resampleAtInterval(rawPoints, minStrokePixels / 3)
  const strokes = classifyDirectionalStrokes(resampled, minStrokePixels, maxStrokes)
  return strokes.join('')
}
