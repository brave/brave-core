/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command.js'
import {makeCanonicalStrokeTemplate, classifyDirectionalStrokeKey} from './directional_gesture_classifier.js'
import {closestCustomGestureDistance, CUSTOM_GESTURE_MAX_MATCH_DISTANCE, CUSTOM_GESTURE_SHAPE_CONFLICT_MAX_DISTANCE, distanceToShape} from './custom_gesture_geometry.js'
import type {Point} from './custom_gesture_geometry.js'
import type {CustomGestureEntry} from './custom_gesture_util.js'
import type {DirectionalGestureMappingActionRow} from './directional_gesture_mapping_util.js'

// The mapped directional key `points` would classify to, only if its own
// shape also resembles that key closely enough to win (`ResolveGestureMatch`).
export function matchedDirectionalGestureKey(
  points: Point[], minStrokePixels: number, minStrokeStraightness: number,
  maxDirectionalStrokes: number,
  directionalGestureMappingActionRows: DirectionalGestureMappingActionRow[]):
  string | null {
  const key = classifyDirectionalStrokeKey(
    points, minStrokePixels, minStrokeStraightness, maxDirectionalStrokes)
  if (!key) {
    return null
  }
  const row = directionalGestureMappingActionRows.find(
    (mappingRow) => mappingRow.key === key)
  if (row === undefined ||
      row.actionRows[0].command === GestureCommand.UNASSIGNED) {
    return null
  }
  const directionalDistance =
    distanceToShape(points, makeCanonicalStrokeTemplate(key))
  return directionalDistance <= CUSTOM_GESTURE_MAX_MATCH_DISTANCE ? key : null
}

// True if any saved custom gesture's drawn gesture_path would classify to the
// directional mapping keyed by `key`, making that gesture unreachable.
export function anyCustomGestureMatchesDirectionalGestureKey(
  key: string, existingCustomGestures: CustomGestureEntry[],
  minStrokePixels: number, minStrokeStraightness: number,
  maxDirectionalStrokes: number,
  directionalGestureMappingActionRows: DirectionalGestureMappingActionRow[]):
  boolean {
  return existingCustomGestures.some((gesture) => matchedDirectionalGestureKey(
    gesture.points, minStrokePixels, minStrokeStraightness,
    maxDirectionalStrokes, directionalGestureMappingActionRows) === key)
}

// Why this custom gesture's drawn gesture_path conflicts with another's, or '' if
// it does not.
export function customGestureShapeConflictDescription(
  id: string, existingCustomGestures: CustomGestureEntry[],
  minStrokePixels: number, minStrokeStraightness: number,
  maxDirectionalStrokes: number,
  directionalGestureMappingActionRows: DirectionalGestureMappingActionRow[],
  i18n: (key: string, ...args: Array<string | number>) => string): string {
  const entry = existingCustomGestures.find((e) => e.id === id)
  if (!entry) {
    return ''
  }
  const directionalGestureKey = matchedDirectionalGestureKey(
    entry.points, minStrokePixels, minStrokeStraightness,
    maxDirectionalStrokes, directionalGestureMappingActionRows)
  if (directionalGestureKey) {
    return i18n(
      'braveGesturesShapeConflictDirectionalSublabel', directionalGestureKey)
  }
  const others = existingCustomGestures.filter((e) => e.id !== id)
  const distance = closestCustomGestureDistance(entry.points, others)
  if (distance !== null &&
    distance < CUSTOM_GESTURE_SHAPE_CONFLICT_MAX_DISTANCE) {
    return i18n('braveGesturesShapeConflictSublabel')
  }
  return ''
}
