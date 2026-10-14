/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  SmartGestureContentType, buildSmartGestureMappingActionRows, smartGestureKeyLabel
} from './smart_gesture_mapping_util'
import {GestureCommand} from './gesture_command'
import {SmartGestureSelectedTextSearchCommandParamType} from './smart_gesture_selected_text_search_command_param'
import {FAKE_GESTURE_COMMAND} from './test/gesture_test_util'
import type {ActionDict} from './gesture_action_util'

describe('buildSmartGestureMappingActionRows', () => {
  test('should build one row per direction for the given content type', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.LINK, undefined)

    expect(rows.map((row) => row.key)).toEqual(
      ['link:up', 'link:down', 'link:left', 'link:right'])
  })

  test('should default link:up to Open Link In', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.LINK, undefined)

    const upRow = rows.find((row) => row.key === 'link:up')
    expect(upRow?.actionRows[0].command).toBe(GestureCommand.OPEN_LINK_IN)
  })

  test('should default selected_text:up to Search Selected Text', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.SELECTED_TEXT, undefined)

    const upRow = rows.find((row) => row.key === 'selected_text:up')
    expect(upRow?.actionRows[0].command).toBe(
      GestureCommand.SEARCH_SELECTED_TEXT)
  })

  test('should default image:left to Save Image', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.IMAGE, undefined)

    const leftRow = rows.find((row) => row.key === 'image:left')
    expect(leftRow?.actionRows[0].command).toBe(GestureCommand.SAVE_IMAGE)
  })

  test('should default link:right to Copy Link Address', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.LINK, undefined)

    const rightRow = rows.find((row) => row.key === 'link:right')
    expect(rightRow?.actionRows[0].command).toBe(
      GestureCommand.COPY_LINK_ADDRESS)
  })

  test('should default selected_text:right to Copy Selected Text', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.SELECTED_TEXT, undefined)

    const rightRow = rows.find((row) => row.key === 'selected_text:right')
    expect(rightRow?.actionRows[0].command).toBe(
      GestureCommand.COPY_SELECTED_TEXT)
  })

  test('should default image:right to Copy Image', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.IMAGE, undefined)

    const rightRow = rows.find((row) => row.key === 'image:right')
    expect(rightRow?.actionRows[0].command).toBe(GestureCommand.COPY_IMAGE)
  })

  test('should default selected_text:down to Brave Ask', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.SELECTED_TEXT, undefined)

    const downRow = rows.find((row) => row.key === 'selected_text:down')
    expect(downRow?.actionRows[0].command).toBe(
      GestureCommand.SEARCH_SELECTED_TEXT)
    expect(downRow?.actionRows[0].openSettingsPage).toBe(
      SmartGestureSelectedTextSearchCommandParamType.BRAVE_ASK)
  })

  test('should leave selected_text:left unassigned', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.SELECTED_TEXT, undefined)

    const leftRow = rows.find((row) => row.key === 'selected_text:left')
    expect(leftRow?.actionRows[0].command).toBe(GestureCommand.UNASSIGNED)
  })

  test('should map link and image down to a new window the same way', () => {
    const linkRows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.LINK, undefined)
    const imageRows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.IMAGE, undefined)

    const linkDownRow = linkRows.find((row) => row.key === 'link:down')
    const imageDownRow = imageRows.find((row) => row.key === 'image:down')
    expect(linkDownRow?.actionRows[0].openSettingsPage).toBe(
      2 /* SmartGestureOpenInCommandParamType.NEW_WINDOW */)
    expect(imageDownRow?.actionRows[0].openSettingsPage).toBe(
      2 /* SmartGestureOpenInCommandParamType.NEW_WINDOW */)
  })

  test('should set link:up\'s destination to a new background tab', () => {
    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.LINK, undefined)

    const upRow = rows.find((row) => row.key === 'link:up')
    expect(upRow?.actionRows[0].openSettingsPage).toBe(
      0 /* SmartGestureOpenInCommandParamType.NEW_BACKGROUND_TAB */)
  })

  test('should use the overridden command when present', () => {
    const overrides: Record<string, ActionDict[]> =
      {'link:left': [{command: FAKE_GESTURE_COMMAND}]}

    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.LINK, overrides)

    const leftRow = rows.find((row) => row.key === 'link:left')
    expect(leftRow?.actionRows[0].command).toBe(FAKE_GESTURE_COMMAND)
  })

  test('should leave other keys at their defaults when one key is overridden', () => {
    const overrides: Record<string, ActionDict[]> =
      {'link:left': [{command: FAKE_GESTURE_COMMAND}]}

    const rows = buildSmartGestureMappingActionRows(
      SmartGestureContentType.LINK, overrides)

    const upRow = rows.find((row) => row.key === 'link:up')
    expect(upRow?.actionRows[0].command).toBe(GestureCommand.OPEN_LINK_IN)
  })
})

describe('smartGestureKeyLabel', () => {
  test('should format a readable label for a link key', () => {
    expect(smartGestureKeyLabel('link:right')).toBe('Link →')
  })

  test('should format a readable label for a selected text key', () => {
    expect(smartGestureKeyLabel('selected_text:up')).toBe('Selected text ↑')
  })

  test('should format a readable label for an image key', () => {
    expect(smartGestureKeyLabel('image:down')).toBe('Image ↓')
  })
})
