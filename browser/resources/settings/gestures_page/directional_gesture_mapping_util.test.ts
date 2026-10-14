/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {buildDirectionalGestureMappingActionRows} from
  './directional_gesture_mapping_util'
import {gestureKeyToIconNames} from './directional_gesture_icons'
import {FAKE_GESTURE_COMMAND} from './test/gesture_test_util'
import type {ActionDict} from './gesture_action_util'

describe('buildDirectionalGestureMappingActionRows', () => {
  test('should build one row per default directional key', () => {
    const rows = buildDirectionalGestureMappingActionRows(undefined)

    expect(rows.map((row) => row.key)).toEqual([
      '↑', '↓', '←', '→', '↓→', '↓↑', '←↑', '→↓', '→↑', '↑↓', '↑→',
    ])
  })

  test('should use the key\'s default command when there is no override', () => {
    const rows = buildDirectionalGestureMappingActionRows(undefined)

    const downRow = rows.find((row) => row.key === '↓')
    expect(downRow?.actionRows[0].command).toBe(34014 /* IDC_NEW_TAB */)
  })

  test('should use the overridden command when present', () => {
    const overrides: Record<string, ActionDict[]> =
      {'↓': [{command: FAKE_GESTURE_COMMAND}]}

    const rows = buildDirectionalGestureMappingActionRows(overrides)

    const downRow = rows.find((row) => row.key === '↓')
    expect(downRow?.actionRows[0].command).toBe(FAKE_GESTURE_COMMAND)
  })

  test('should leave other keys at their defaults when one key is overridden', () => {
    const overrides: Record<string, ActionDict[]> =
      {'↓': [{command: FAKE_GESTURE_COMMAND}]}

    const rows = buildDirectionalGestureMappingActionRows(overrides)

    const backRow = rows.find((row) => row.key === '←')
    expect(backRow?.actionRows[0].command).toBe(33000 /* IDC_BACK */)
  })

  test('should include the correct icons for each directional gesture key', () => {
    const rows = buildDirectionalGestureMappingActionRows(undefined)

    const downRightRow = rows.find((row) => row.key === '↓→')
    expect(downRightRow?.icons).toEqual(gestureKeyToIconNames('↓→'))
  })
})
