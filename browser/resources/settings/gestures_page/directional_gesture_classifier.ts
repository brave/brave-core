/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {Point} from './custom_gesture_geometry.js'

// Mirrors `DirectionalGestureRecognizer`'s classification in
// brave/components/gestures/core/common/directional/directional_gesture_recognizer.cc
// and `DirectionToKeyCode` in directional_gesture_util.cc. Direction
// classification itself has no timing dependency there (only the idle-hold
// timeout does, which only matters for an abandoned, not-yet-finished draw),
// so replaying a finished raw point sequence through this batch version
// reproduces exactly what the live incremental recognizer would have
// resolved to. Used only for a pre-save "this looks like a mapped
// directional gesture, so it would never actually run" warning; actual
// directional matching is always done in C++ at gesture time.

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

// Classifies `rawPoints` (in the same raw pixel units `minStrokePixels` is
// calibrated in, not resampled or normalized) into the stroke-key string
// `DirectionalGestureMapping` looks mappings up by, e.g. "←" or "↓→".
export function classifyDirectionalStrokeKey(
    rawPoints: Point[], minStrokePixels: number, maxStrokes: number):
    string {
  if (rawPoints.length === 0) {
    return ''
  }

  const strokes: string[] = []
  let strokeOrigin = rawPoints[0]
  // A direction change not yet committed as a stroke, mirroring
  // `pending_direction_type_`/`pending_origin_`: a 90-degree turn briefly
  // crosses the diagonal octant between the two strokes, so a candidate
  // direction is only committed once it's sustained on its own for
  // `minStrokePixels`.
  let pendingDirection: string|null = null
  let pendingOrigin: Point = {x: 0, y: 0}

  for (let i = 1; i < rawPoints.length && strokes.length < maxStrokes; ++i) {
    const location = rawPoints[i]
    const direction = maybeMovementDirection(
        strokeOrigin, location, minStrokePixels)
    if (direction === null) {
      continue
    }

    if (strokes.length === 0 || direction === strokes[strokes.length - 1]) {
      pendingDirection = null
      if (strokes.length === 0) {
        strokes.push(direction)
      }
      strokeOrigin = location
      continue
    }

    if (pendingDirection !== direction) {
      pendingDirection = direction
      pendingOrigin = location
      continue
    }
    if (maybeMovementDirection(pendingOrigin, location, minStrokePixels) !==
        direction) {
      continue
    }

    strokes.push(direction)
    pendingDirection = null
    strokeOrigin = location
  }

  return strokes.join('')
}
