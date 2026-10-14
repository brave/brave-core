/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {buildCustomGestureActionRows} from './custom_gesture_util'
import {GestureCommand} from './gesture_command'
import {FAKE_GESTURE_COMMAND} from './test/gesture_test_util'
import type {CustomGestureEntry} from './custom_gesture_util'

describe('buildCustomGestureActionRows', () => {
  test('should return an empty array when there are no custom gestures', () => {
    expect(buildCustomGestureActionRows(undefined)).toEqual([])
  })

  test('should carry over each custom gesture\'s id', () => {
    const customGestures: CustomGestureEntry[] =
      [{id: 'first', points: [{x: 0, y: 0}, {x: 0, y: 100}]}]

    const rows = buildCustomGestureActionRows(customGestures)

    expect(rows[0].id).toBe('first')
  })

  test('should default an unassigned custom gesture\'s action', () => {
    const customGestures: CustomGestureEntry[] =
      [{id: 'first', points: [{x: 0, y: 0}, {x: 0, y: 100}]}]

    const rows = buildCustomGestureActionRows(customGestures)

    expect(rows[0].actionRows[0].command).toBe(GestureCommand.UNASSIGNED)
  })

  test('should use the stored action when one is present', () => {
    const customGestures: CustomGestureEntry[] = [{
      id: 'first', points: [{x: 0, y: 0}, {x: 0, y: 100}],
      actions: [{command: FAKE_GESTURE_COMMAND}],
    }]

    const rows = buildCustomGestureActionRows(customGestures)

    expect(rows[0].actionRows[0].command).toBe(FAKE_GESTURE_COMMAND)
  })

  test('should build a non-empty preview path for a gesture with enough points', () => {
    const customGestures: CustomGestureEntry[] =
      [{id: 'first', points: [{x: 0, y: 0}, {x: 0, y: 100}]}]

    const rows = buildCustomGestureActionRows(customGestures)

    expect(rows[0].previewSvgPathData).not.toBe('')
  })

  test('should leave the preview path empty for a gesture with fewer than two points', () => {
    const customGestures: CustomGestureEntry[] =
      [{id: 'first', points: [{x: 0, y: 0}]}]

    const rows = buildCustomGestureActionRows(customGestures)

    expect(rows[0].previewSvgPathData).toBe('')
  })
})
