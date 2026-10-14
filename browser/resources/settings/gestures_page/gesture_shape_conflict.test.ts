/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command'
import {
  anyCustomGestureMatchesDirectionalGestureKey,
  customGestureShapeConflictDescription,
} from './gesture_shape_conflict'
import {FAKE_GESTURE_COMMAND} from './test/gesture_test_util'
import type {DirectionalGestureMappingActionRow} from './directional_gesture_mapping_util'
import type {CustomGestureEntry} from './custom_gesture_util'

const MIN_STROKE_PIXELS = 30
const MIN_STROKE_STRAIGHTNESS = 0.9
const MAX_DIRECTIONAL_STROKES = 4

function makeMappingRow(key: string, command: number):
  DirectionalGestureMappingActionRow {
  return {
    key,
    icons: [],
    actionRows: [{command, textParam: '', openSettingsPage: 0}],
  }
}

const i18n = (key: string, ...args: Array<string | number>): string =>
  args.length > 0 ? `${key}:${args.join(',')}` : key

describe('customGestureShapeConflictDescription', () => {
  test('should not report a conflict when the shape only resembles the beginning of a directional key', () => {
    // Classification still finds "↑" from the initial stroke, but the shape
    // looks nothing like it once oscillation dominates; distance is checked.
    const oscillatingPoints = [
      {x: 0, y: 0}, {x: 0, y: -30}, {x: 0, y: -60},
      {x: 20, y: -60}, {x: -20, y: -60}, {x: 20, y: -60}, {x: -20, y: -60},
      {x: 20, y: -60}, {x: -20, y: -60}, {x: 20, y: -60}, {x: -20, y: -60},
      {x: 20, y: -60}, {x: -20, y: -60},
    ]
    const existingCustomGestures: CustomGestureEntry[] =
      [{id: 'oscillating', points: oscillatingPoints}]
    const directionalGestureMappingActionRows = [
      makeMappingRow('↑', 56223 /* IDC_SCROLL_TAB_TO_TOP */),
    ]

    const description = customGestureShapeConflictDescription(
      'oscillating', existingCustomGestures, MIN_STROKE_PIXELS,
      MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_STROKES,
      directionalGestureMappingActionRows, i18n)

    expect(description).toBe('')
  })

  test('should report a conflict when the shape resembles the entire directional key', () => {
    // Classifies as "↓" and, as a shape, is essentially identical to
    // "↓"'s own idealized template too.
    const straightDownPoints = [{x: 0, y: 0}, {x: 0, y: 100}]
    const existingCustomGestures: CustomGestureEntry[] =
      [{id: 'straight-down', points: straightDownPoints}]
    const directionalGestureMappingActionRows = [
      makeMappingRow('↓', FAKE_GESTURE_COMMAND),
    ]

    const description = customGestureShapeConflictDescription(
      'straight-down', existingCustomGestures, MIN_STROKE_PIXELS,
      MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_STROKES,
      directionalGestureMappingActionRows, i18n)

    expect(description).toBe('braveGesturesShapeConflictDirectionalSublabel:↓')
  })

  test('should not report a conflict with an unassigned directional key', () => {
    const straightDownPoints = [{x: 0, y: 0}, {x: 0, y: 100}]
    const existingCustomGestures: CustomGestureEntry[] =
      [{id: 'straight-down', points: straightDownPoints}]
    const directionalGestureMappingActionRows = [
      makeMappingRow('↓', GestureCommand.UNASSIGNED),
    ]

    const description = customGestureShapeConflictDescription(
      'straight-down', existingCustomGestures, MIN_STROKE_PIXELS,
      MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_STROKES,
      directionalGestureMappingActionRows, i18n)

    expect(description).toBe('')
  })

  test('should report a conflict with a similarly shaped custom gesture', () => {
    const existingCustomGestures: CustomGestureEntry[] = [
      {id: 'first', points: [{x: 0, y: 0}, {x: 50, y: 100}, {x: 100, y: 0}]},
      {id: 'second', points: [{x: 0, y: 0}, {x: 50, y: 99}, {x: 100, y: 1}]},
    ]

    const description = customGestureShapeConflictDescription(
      'first', existingCustomGestures, MIN_STROKE_PIXELS,
      MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_STROKES, [], i18n)

    expect(description).toBe('braveGesturesShapeConflictSublabel')
  })

  test('should not report a conflict when distance falls between the match and shape-conflict thresholds', () => {
    // This distance (about 0.121) is below the old shared threshold (0.14)
    // but above the stricter decoupled one (0.1), so it must not warn.
    const existingCustomGestures: CustomGestureEntry[] = [
      {id: 'first', points: [{x: 0, y: 0}, {x: 50, y: 100}, {x: 100, y: 0}]},
      {id: 'second', points: [{x: 0, y: 0}, {x: 50, y: 75}, {x: 100, y: 0}]},
    ]

    const description = customGestureShapeConflictDescription(
      'first', existingCustomGestures, MIN_STROKE_PIXELS,
      MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_STROKES, [], i18n)

    expect(description).toBe('')
  })

  test('should return an empty description when the gesture id is not found', () => {
    expect(customGestureShapeConflictDescription(
      'missing', [], MIN_STROKE_PIXELS, MIN_STROKE_STRAIGHTNESS,
      MAX_DIRECTIONAL_STROKES, [], i18n)).toBe('')
  })

  test('should return an empty description when there are no other custom gestures', () => {
    const existingCustomGestures: CustomGestureEntry[] =
      [{id: 'only', points: [{x: 0, y: 0}, {x: 0, y: 100}]}]

    const description = customGestureShapeConflictDescription(
      'only', existingCustomGestures, MIN_STROKE_PIXELS,
      MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_STROKES, [], i18n)

    expect(description).toBe('')
  })

})

describe('anyCustomGestureMatchesDirectionalGestureKey', () => {
  test('should report a match when a custom gesture both classifies as and resembles the directional key', () => {
    const existingCustomGestures: CustomGestureEntry[] =
      [{id: 'straight-down', points: [{x: 0, y: 0}, {x: 0, y: 100}]}]
    const directionalGestureMappingActionRows = [
      makeMappingRow('↓', FAKE_GESTURE_COMMAND),
    ]

    expect(anyCustomGestureMatchesDirectionalGestureKey(
      '↓', existingCustomGestures, MIN_STROKE_PIXELS, MIN_STROKE_STRAIGHTNESS,
      MAX_DIRECTIONAL_STROKES, directionalGestureMappingActionRows))
      .toBe(true)
  })

  test('should report no match when no custom gesture matches the directional key', () => {
    const existingCustomGestures: CustomGestureEntry[] =
      [{id: 'straight-down', points: [{x: 0, y: 0}, {x: 0, y: 100}]}]
    const directionalGestureMappingActionRows = [
      makeMappingRow('↓', FAKE_GESTURE_COMMAND),
    ]

    expect(anyCustomGestureMatchesDirectionalGestureKey(
      '→', existingCustomGestures, MIN_STROKE_PIXELS, MIN_STROKE_STRAIGHTNESS,
      MAX_DIRECTIONAL_STROKES, directionalGestureMappingActionRows))
      .toBe(false)
  })

  test('should report no match when a custom gesture classifies as the directional key but does not resemble it', () => {
    const oscillatingPoints = [
      {x: 0, y: 0}, {x: 0, y: -30}, {x: 0, y: -60},
      {x: 20, y: -60}, {x: -20, y: -60}, {x: 20, y: -60}, {x: -20, y: -60},
      {x: 20, y: -60}, {x: -20, y: -60}, {x: 20, y: -60}, {x: -20, y: -60},
      {x: 20, y: -60}, {x: -20, y: -60},
    ]
    const existingCustomGestures: CustomGestureEntry[] =
      [{id: 'oscillating', points: oscillatingPoints}]
    const directionalGestureMappingActionRows = [
      makeMappingRow('↑', 56223 /* IDC_SCROLL_TAB_TO_TOP */),
    ]

    expect(anyCustomGestureMatchesDirectionalGestureKey(
      '↑', existingCustomGestures, MIN_STROKE_PIXELS, MIN_STROKE_STRAIGHTNESS,
      MAX_DIRECTIONAL_STROKES, directionalGestureMappingActionRows))
      .toBe(false)
  })

  test('should report no match when the matching directional key is unassigned', () => {
    const existingCustomGestures: CustomGestureEntry[] =
      [{id: 'straight-down', points: [{x: 0, y: 0}, {x: 0, y: 100}]}]
    const directionalGestureMappingActionRows = [
      makeMappingRow('↓', GestureCommand.UNASSIGNED),
    ]

    expect(anyCustomGestureMatchesDirectionalGestureKey(
      '↓', existingCustomGestures, MIN_STROKE_PIXELS, MIN_STROKE_STRAIGHTNESS,
      MAX_DIRECTIONAL_STROKES, directionalGestureMappingActionRows))
      .toBe(false)
  })
})
