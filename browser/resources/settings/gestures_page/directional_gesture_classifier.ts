/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {Point} from './custom_gesture_geometry.js'

// Mirrors `ResampleGesturePathAtInterval`/`ClassifyDirectionalGestureStrokes`;
// used only for a pre-save warning, not actual matching, which is always done
// in C++.

// Clockwise angular order starting from "right" (screen angle 0), matching
// `kDirectionsByAngle`.
const DIRECTIONS_BY_ANGLE = ['→', '↘', '↓', '↙', '←', '↖', '↑', '↗']

const DIAGONAL_UNIT = Math.SQRT1_2

// Unit displacement for each direction glyph, matching `DIRECTIONS_BY_ANGLE`.
const UNIT_VECTOR_BY_DIRECTION: {[key: string]: Point} = {
  '→': {x: 1, y: 0},
  '↘': {x: DIAGONAL_UNIT, y: DIAGONAL_UNIT},
  '↓': {x: 0, y: 1},
  '↙': {x: -DIAGONAL_UNIT, y: DIAGONAL_UNIT},
  '←': {x: -1, y: 0},
  '↖': {x: -DIAGONAL_UNIT, y: -DIAGONAL_UNIT},
  '↑': {x: 0, y: -1},
  '↗': {x: DIAGONAL_UNIT, y: -DIAGONAL_UNIT},
}

// Makes a canonical polyline for `key` (e.g. "↓→"): each stroke glyph
// becomes a unit-length straight segment in its direction, chained end to
// end starting at the origin. Mirrors
// `DirectionalGestureStrokesToCanonicalPath` on the C++ side, letting a
// resolved directional-stroke key be compared against a custom gesture's
// drawn gesture_path on the same normalized distance scale (see
// `distanceToShape` in `custom_gesture_geometry.ts`), instead of any
// classification at all being treated as an automatic conflict.
export function makeCanonicalStrokeTemplate(key: string): Point[] {
  const gesturePath: Point[] = [{ x: 0, y: 0 }]
  let current = {x: 0, y: 0}
  for (const glyph of key) {
    const unit = UNIT_VECTOR_BY_DIRECTION[glyph]
    current = {x: current.x + unit.x, y: current.y + unit.y}
    gesturePath.push(current)
  }
  return gesturePath
}

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

// Mirrors `ResampleGesturePathAtInterval`, resampling `gesture_path` evenly by arc length,
// so classification does not depend on how unevenly points were sampled.
function resampleAtInterval(gesture_path: Point[], intervalPixels: number): Point[] {
  if (gesture_path.length === 0) {
    return []
  }
  const resampled: Point[] = [gesture_path[0]]
  if (gesture_path.length < 2 || intervalPixels <= 0) {
    return resampled
  }

  // Carries over leftover distance, so intervals stay consistent across
  // the original points' own boundaries.
  let accumulated = 0
  let lastPoint = gesture_path[0]
  for (let i = 1; i < gesture_path.length; ++i) {
    const current = gesture_path[i]
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
      // Advances from the interpolated point, not the original segment's
      // start, so a long segment resamples into evenly-spaced points.
      lastPoint = newPoint
      accumulated = 0
    }
    accumulated += segmentLength
    lastPoint = current
  }
  const last = resampled[resampled.length - 1]
  const end = gesture_path[gesture_path.length - 1]
  if (last.x !== end.x || last.y !== end.y) {
    resampled.push(end)
  }
  return resampled
}

// Mirrors `ClassifyDirectionalGestureStrokes`. `strokeOrigin` is the start of
// the current committed stroke, advancing on every commit; `pendingOrigin` is
// the start of the current pending candidate, advancing whenever the
// candidate direction changes. A single shared anchor (as an earlier
// version of this had) reads a slowly-drifting aggregate bearing on a
// curved gesture_path, which can disagree with the real local tangent for the rest
// of the gesture_path and prevent any further stroke from ever committing -- e.g. a
// hand-drawn oval collapsing to a single stroke.
function classifyDirectionalStrokes(
  gesture_path: Point[], minStrokePixels: number, minStrokeStraightness: number,
    maxStrokes: number): string[] {
  const strokes: string[] = []
  if (gesture_path.length === 0) {
    return strokes
  }

  let strokeOrigin = gesture_path[0]
  let lastPoint = gesture_path[0]
  let pendingDirection: string|null = null
  let pendingOrigin = gesture_path[0]

  // Actual distance traveled since `strokeOrigin`, always running (unlike
  // `arcLengthSinceDeparture` below, which only accumulates while diverged).
  // Gates the "continuing the current stroke" branch: without this, any
  // point that happens to land back within `minStrokePixels` of
  // `strokeOrigin` on the committed octant is accepted as a genuine
  // continuation, even if it only got there by looping or wandering back
  // around -- a coincidental geometric realignment, not real continued
  // travel in that direction.
  let arcLengthSinceStrokeOrigin = 0

  // Straightness-only state, independent of `strokeOrigin`/`pendingOrigin`
  // above. `departureOrigin` is set once, the first time movement diverges
  // from the committed direction (or there's no stroke yet), and is only
  // cleared by a return to that direction or by a commit -- never by a
  // later flip of `pendingDirection` -- so wandering through several
  // directions before settling is still charged against straightness
  // instead of being forgotten. Measuring from `strokeOrigin` instead would
  // double it as the straightness anchor too, but `strokeOrigin` only
  // advances on points that independently reclear `minStrokePixels` on
  // their own, so it can lag the real corner by almost a full
  // `minStrokePixels` and contaminate the next stroke's straightness with
  // leftover travel that geometrically belongs to the stroke before it.
  let hasDeparture = false
  let departureOrigin = gesture_path[0]
  let arcLengthSinceDeparture = 0

  for (let i = 1; i < gesture_path.length && strokes.length < maxStrokes; ++i) {
    const location = gesture_path[i]
    const segmentLength =
        Math.hypot(location.x - lastPoint.x, location.y - lastPoint.y)
    lastPoint = location
    arcLengthSinceStrokeOrigin += segmentLength

    const direction =
        maybeMovementDirection(strokeOrigin, location, minStrokePixels)
    if (direction === null) {
      if (hasDeparture) {
        arcLengthSinceDeparture += segmentLength
      }
      continue
    }

    if (strokes.length > 0 && direction === strokes[strokes.length - 1]) {
      // A straight-line displacement back onto the committed direction can
      // still come from a gesture_path that looped or wandered to get there;
      // require it to also dominate the actual distance traveled since
      // `strokeOrigin` before accepting it as a genuine continuation,
      // rather than a coincidental geometric realignment.
      const continuingStraightness = arcLengthSinceStrokeOrigin > 0
          ? Math.hypot(
                location.x - strokeOrigin.x, location.y - strokeOrigin.y) /
              arcLengthSinceStrokeOrigin
          : 1
      if (continuingStraightness >= minStrokeStraightness) {
        pendingDirection = null
        strokeOrigin = location
        arcLengthSinceStrokeOrigin = 0
        hasDeparture = false
        arcLengthSinceDeparture = 0
        continue
      }
      // Coincidental realignment only; charge the wander distance against
      // straightness like any other non-qualifying point, but leave the
      // pending candidate and departure state untouched.
      if (hasDeparture) {
        arcLengthSinceDeparture += segmentLength
      }
      continue
    }

    // Diverged from the committed direction, or there's no stroke yet.
    // Anchor straightness at the first point of this divergence; later
    // flips of `pendingDirection` below do not move this anchor.
    if (!hasDeparture) {
      hasDeparture = true
      departureOrigin = location
      arcLengthSinceDeparture = 0
    } else {
      arcLengthSinceDeparture += segmentLength
    }

    // `direction` differs from the stroke in progress, or there's no
    // stroke yet. Don't commit it until it's sustained on its own for
    // `minStrokePixels`.
    if (pendingDirection !== direction) {
      pendingDirection = direction
      pendingOrigin = location
      continue
    }
    if (maybeMovementDirection(pendingOrigin, location, minStrokePixels) !==
        direction) {
      continue
    }

    // A straight-line displacement that clears `minStrokePixels` can still
    // come from a gesture_path that wandered to get there (e.g. a loop briefly
    // drifting in one direction); requiring the displacement to also
    // dominate the actual distance traveled since `departureOrigin` rejects
    // those, while a real swipe -- which travels nearly straight to where
    // it ends up -- passes easily.
    const straightness = arcLengthSinceDeparture > 0
        ? Math.hypot(
              location.x - departureOrigin.x, location.y - departureOrigin.y) /
            arcLengthSinceDeparture
        : 1
    if (straightness < minStrokeStraightness) {
      continue
    }

    strokes.push(direction)
    pendingDirection = null
    strokeOrigin = location
    arcLengthSinceStrokeOrigin = 0
    hasDeparture = false
    arcLengthSinceDeparture = 0
  }

  return strokes
}

// Classifies `rawPoints`, in the same raw pixel units `minStrokePixels`
// uses, into a mapping lookup key such as "←" or "↓→".
export function classifyDirectionalStrokeKey(
    rawPoints: Point[], minStrokePixels: number, minStrokeStraightness: number,
    maxStrokes: number): string {
  const resampled = resampleAtInterval(rawPoints, minStrokePixels / 3)
  const strokes = classifyDirectionalStrokes(
      resampled, minStrokePixels, minStrokeStraightness, maxStrokes)
  return strokes.join('')
}
