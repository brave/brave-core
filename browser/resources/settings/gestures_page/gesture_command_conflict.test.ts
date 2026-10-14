/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command'
import {findConflictingGestures} from './gesture_command_conflict'
import type {ActionRow, MouseRockerActionRow, MouseWheelActionRow} from
  './gesture_rows'
import type {CustomGestureActionRow} from './custom_gesture_util'
import type {DirectionalGestureMappingActionRow} from
  './directional_gesture_mapping_util'
import type {SmartGestureMappingActionRow} from './smart_gesture_mapping_util'
import {FAKE_GESTURE_COMMAND} from './test/gesture_test_util'

function makeActionRows(command: number): ActionRow[] {
  return [{command, textParam: '', openSettingsPage: 0}]
}

function makeMappingRow(key: string, command: number):
  DirectionalGestureMappingActionRow {
  return {key, icons: [], actionRows: makeActionRows(command)}
}

function makeSmartGestureRow(key: string, command: number):
  SmartGestureMappingActionRow {
  return {key, direction: '', icon: '', actionRows: makeActionRows(command)}
}

function makeCustomGestureRow(id: string, command: number):
  CustomGestureActionRow {
  return {id, previewSvgPathData: '', actionRows: makeActionRows(command)}
}

function makeMouseRockerRow(prefName: string, command: number):
  MouseRockerActionRow {
  return {prefName, actionRows: makeActionRows(command)}
}

function makeMouseWheelRow(prefName: string, command: number):
  MouseWheelActionRow {
  return {prefName, actionRows: makeActionRows(command)}
}

describe('findConflictingGestures', () => {
  test('should find no conflicts when the action is unassigned', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(GestureCommand.UNASSIGNED), {},
      [makeMappingRow('↓', GestureCommand.UNASSIGNED)],
      [] /* customGestureActionRows */, [] /* smartGestureMappingActionRows */,
      [] /* mouseRockerActionRows */, [] /* mouseWheelActionRows */)

    expect(conflicts).toEqual({
      keys: [], smartGestureKeys: [], customGestureCount: 0,
      mouseRockerCount: 0, mouseWheelCount: 0
    })
  })

  test('should find another directional gesture mapped to the same command', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {directionalGestureMappingKey: '↑'},
      [makeMappingRow('↑', FAKE_GESTURE_COMMAND),
       makeMappingRow('↓', FAKE_GESTURE_COMMAND)],
      [] /* customGestureActionRows */, [] /* smartGestureMappingActionRows */,
      [] /* mouseRockerActionRows */, [] /* mouseWheelActionRows */)

    expect(conflicts.keys).toEqual(['↓'])
  })

  test('should not find a conflict when the other directional gesture is mapped to a different command', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {directionalGestureMappingKey: '↑'},
      [makeMappingRow('↑', FAKE_GESTURE_COMMAND),
       makeMappingRow('↓', GestureCommand.SWITCH_TO_TAB)],
      [] /* customGestureActionRows */, [] /* smartGestureMappingActionRows */,
      [] /* mouseRockerActionRows */, [] /* mouseWheelActionRows */)

    expect(conflicts.keys).toEqual([])
  })

  test('should count another custom gesture mapped to the same command', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {customGestureId: 'first'},
      [] /* directionalGestureMappingActionRows */,
      [makeCustomGestureRow('first', FAKE_GESTURE_COMMAND),
       makeCustomGestureRow('second', FAKE_GESTURE_COMMAND)],
      [] /* smartGestureMappingActionRows */, [] /* mouseRockerActionRows */,
      [] /* mouseWheelActionRows */)

    expect(conflicts.customGestureCount).toBe(1)
  })

  test('should find another smart gesture mapped to the same command', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {smartGestureKey: 'link:right'},
      [] /* directionalGestureMappingActionRows */,
      [] /* customGestureActionRows */,
      [makeSmartGestureRow('link:right', FAKE_GESTURE_COMMAND),
       makeSmartGestureRow('image:down', FAKE_GESTURE_COMMAND)],
      [] /* mouseRockerActionRows */, [] /* mouseWheelActionRows */)

    expect(conflicts.smartGestureKeys).toEqual(['Image ↓'])
  })

  test('should count a smart gesture mapped to the same command as a directional gesture', () => {
    // Directional strokes and smart gestures are both read from the same
    // drawn mouse path, competing to explain the same physical gesture, so
    // sharing a command between them is a real conflict.
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {directionalGestureMappingKey: '↑'},
      [makeMappingRow('↑', FAKE_GESTURE_COMMAND)],
      [] /* customGestureActionRows */,
      [makeSmartGestureRow('link:right', FAKE_GESTURE_COMMAND)],
      [] /* mouseRockerActionRows */, [] /* mouseWheelActionRows */)

    expect(conflicts.smartGestureKeys).toEqual(['Link →'])
  })

  test('should count another mouse rocker gesture mapped to the same command', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {mouseRockerPrefName: 'left'},
      [] /* directionalGestureMappingActionRows */,
      [] /* customGestureActionRows */, [] /* smartGestureMappingActionRows */,
      [makeMouseRockerRow('left', FAKE_GESTURE_COMMAND),
       makeMouseRockerRow('right', FAKE_GESTURE_COMMAND)],
      [] /* mouseWheelActionRows */)

    expect(conflicts.mouseRockerCount).toBe(1)
  })

  test('should count another mouse wheel gesture mapped to the same command', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {mouseWheelPrefName: 'up'},
      [] /* directionalGestureMappingActionRows */,
      [] /* customGestureActionRows */, [] /* smartGestureMappingActionRows */,
      [] /* mouseRockerActionRows */,
      [makeMouseWheelRow('up', FAKE_GESTURE_COMMAND),
       makeMouseWheelRow('down', FAKE_GESTURE_COMMAND)])

    expect(conflicts.mouseWheelCount).toBe(1)
  })

  test('should not count a directional gesture mapped to the same command as a mouse rocker gesture', () => {
    // Directional strokes and mouse rocker clicks fire from different input
    // mechanisms, so sharing a command between them is not a real conflict.
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {mouseRockerPrefName: 'left'},
      [makeMappingRow('↑', FAKE_GESTURE_COMMAND)],
      [] /* customGestureActionRows */, [] /* smartGestureMappingActionRows */,
      [makeMouseRockerRow('left', FAKE_GESTURE_COMMAND)],
      [] /* mouseWheelActionRows */)

    expect(conflicts.keys).toEqual([])
  })

  test('should count a custom gesture mapped to the same command as a directional gesture', () => {
    // Directional strokes and custom gestures are both read from the same
    // drawn mouse path, competing to explain the same physical gesture, so
    // sharing a command between them is a real conflict.
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {directionalGestureMappingKey: '↑'},
      [makeMappingRow('↑', FAKE_GESTURE_COMMAND)],
      [makeCustomGestureRow('custom', FAKE_GESTURE_COMMAND)],
      [] /* smartGestureMappingActionRows */, [] /* mouseRockerActionRows */,
      [] /* mouseWheelActionRows */)

    expect(conflicts.customGestureCount).toBe(1)
  })

  test('should find a directional gesture mapped to the same command as a custom gesture', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {customGestureId: 'custom'},
      [makeMappingRow('↑', FAKE_GESTURE_COMMAND)],
      [makeCustomGestureRow('custom', FAKE_GESTURE_COMMAND)],
      [] /* smartGestureMappingActionRows */, [] /* mouseRockerActionRows */,
      [] /* mouseWheelActionRows */)

    expect(conflicts.keys).toEqual(['↑'])
  })

  test('should not count a mouse rocker gesture mapped to the same command as a mouse wheel gesture', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {mouseWheelPrefName: 'up'},
      [] /* directionalGestureMappingActionRows */,
      [] /* customGestureActionRows */, [] /* smartGestureMappingActionRows */,
      [makeMouseRockerRow('left', FAKE_GESTURE_COMMAND)],
      [makeMouseWheelRow('up', FAKE_GESTURE_COMMAND)])

    expect(conflicts.mouseRockerCount).toBe(0)
  })

  test('should not count a mouse wheel gesture mapped to the same command as a custom gesture', () => {
    const conflicts = findConflictingGestures(
      makeActionRows(FAKE_GESTURE_COMMAND), {customGestureId: 'custom'},
      [] /* directionalGestureMappingActionRows */,
      [makeCustomGestureRow('custom', FAKE_GESTURE_COMMAND)],
      [] /* smartGestureMappingActionRows */, [] /* mouseRockerActionRows */,
      [makeMouseWheelRow('up', FAKE_GESTURE_COMMAND)])

    expect(conflicts.mouseWheelCount).toBe(0)
  })
})
