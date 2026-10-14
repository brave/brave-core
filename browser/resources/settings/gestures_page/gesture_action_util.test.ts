/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  actionRowsFromDicts, applyCommandToActionRows, buildActionRow,
  defaultActionRow, IDC_OPTIONS, moveAction, toActionDict, toActionDicts,
} from './gesture_action_util'
import {OpenSettingsPage} from './open_settings_page'
import {FAKE_GESTURE_COMMAND} from './test/gesture_test_util'
import type {ActionDict} from './gesture_action_util'
import type {ActionRow} from './gesture_rows'

describe('defaultActionRow', () => {
  test('should default the text and settings page values', () => {
    expect(defaultActionRow(FAKE_GESTURE_COMMAND)).toEqual({
      command: FAKE_GESTURE_COMMAND, textParam: '',
      openSettingsPage: OpenSettingsPage.BASIC,
    })
  })
})

describe('actionRowsFromDicts', () => {
  test('should fall back to a single default row when there are no stored actions', () => {
    expect(actionRowsFromDicts(undefined, FAKE_GESTURE_COMMAND))
      .toEqual([defaultActionRow(FAKE_GESTURE_COMMAND)])
  })

  test('should fall back to a single default row when the stored actions list is empty', () => {
    expect(actionRowsFromDicts([], FAKE_GESTURE_COMMAND))
      .toEqual([defaultActionRow(FAKE_GESTURE_COMMAND)])
  })

  test('should convert stored actions and fill missing values with defaults', () => {
    const actions: ActionDict[] = [{command: FAKE_GESTURE_COMMAND}]

    expect(actionRowsFromDicts(actions, FAKE_GESTURE_COMMAND)).toEqual([{
      command: FAKE_GESTURE_COMMAND, textParam: '',
      openSettingsPage: OpenSettingsPage.BASIC,
    }])
  })

  test('should carry over a stored action\'s text value', () => {
    const actions: ActionDict[] = [
      {command: FAKE_GESTURE_COMMAND, text_param: 'https://example.com'},
    ]

    expect(actionRowsFromDicts(actions, FAKE_GESTURE_COMMAND)[0].textParam)
      .toBe('https://example.com')
  })

  test('should carry over a stored action\'s open settings page value', () => {
    const actions: ActionDict[] = [
      {command: IDC_OPTIONS, value_param: OpenSettingsPage.PAYMENTS},
    ]

    expect(actionRowsFromDicts(actions, IDC_OPTIONS)[0].openSettingsPage)
      .toBe(OpenSettingsPage.PAYMENTS)
  })
})

describe('buildActionRow', () => {
  test('should use the pref name and default command when there are no stored actions', () => {
    expect(buildActionRow('pref.name', undefined, FAKE_GESTURE_COMMAND))
      .toEqual({
        prefName: 'pref.name',
        actionRows: [defaultActionRow(FAKE_GESTURE_COMMAND)],
      })
  })

  test('should use the stored actions when present', () => {
    const actions: ActionDict[] = [{command: FAKE_GESTURE_COMMAND}]

    expect(buildActionRow('pref.name', actions, FAKE_GESTURE_COMMAND))
      .toEqual({
        prefName: 'pref.name',
        actionRows: actionRowsFromDicts(actions, FAKE_GESTURE_COMMAND),
      })
  })
})

describe('moveAction', () => {
  const rowA: ActionRow = {command: 1, textParam: '', openSettingsPage: 0}
  const rowB: ActionRow = {command: 2, textParam: '', openSettingsPage: 0}
  const rowC: ActionRow = {command: 3, textParam: '', openSettingsPage: 0}

  test('should leave the order unchanged when dragging a row onto itself', () => {
    const actionRows = [rowA, rowB, rowC]

    expect(moveAction(actionRows, rowB, rowB)).toBe(actionRows)
  })

  test('should insert after the target when moving down', () => {
    expect(moveAction([rowA, rowB, rowC], rowA, rowC))
      .toEqual([rowB, rowC, rowA])
  })

  test('should insert before the target when moving up', () => {
    expect(moveAction([rowA, rowB, rowC], rowC, rowA))
      .toEqual([rowC, rowA, rowB])
  })
})

describe('applyCommandToActionRows', () => {
  test('should default to a single row when there are no existing action rows', () => {
    expect(applyCommandToActionRows(FAKE_GESTURE_COMMAND, undefined))
      .toEqual([defaultActionRow(FAKE_GESTURE_COMMAND)])
  })

  test('should replace the primary row, keeping chained rows', () => {
    const chainedRow: ActionRow =
      {command: FAKE_GESTURE_COMMAND, textParam: '', openSettingsPage: 0}
    const primaryRow: ActionRow =
      {command: 1, textParam: 'kept', openSettingsPage: 0}

    expect(applyCommandToActionRows(FAKE_GESTURE_COMMAND, [primaryRow, chainedRow]))
      .toEqual([defaultActionRow(FAKE_GESTURE_COMMAND), chainedRow])
  })

  test('should clear stale primary-row parameters when changing commands', () => {
    const primaryRow: ActionRow = {
      command: 1, textParam: 'stale url', openSettingsPage: 2,
    }

    expect(applyCommandToActionRows(FAKE_GESTURE_COMMAND, [primaryRow]))
      .toEqual([defaultActionRow(FAKE_GESTURE_COMMAND)])
  })
})

describe('toActionDict', () => {
  test('should serialize the settings page param for IDC_OPTIONS', () => {
    const action: ActionRow = {
      command: IDC_OPTIONS, textParam: 'ignored',
      openSettingsPage: OpenSettingsPage.PAYMENTS,
    }

    expect(toActionDict(action)).toEqual({
      command: IDC_OPTIONS, value_param: OpenSettingsPage.PAYMENTS,
    })
  })

  test('should serialize the text parameter for commands other than IDC_OPTIONS', () => {
    const action: ActionRow = {
      command: FAKE_GESTURE_COMMAND, textParam: 'https://example.com',
      openSettingsPage: OpenSettingsPage.PAYMENTS,
    }

    expect(toActionDict(action)).toEqual({
      command: FAKE_GESTURE_COMMAND, text_param: 'https://example.com',
    })
  })
})

describe('toActionDicts', () => {
  test('should serialize every row in order', () => {
    const actionRows: ActionRow[] = [
      {command: FAKE_GESTURE_COMMAND, textParam: 'a', openSettingsPage: 0},
      {command: FAKE_GESTURE_COMMAND, textParam: 'b', openSettingsPage: 0},
    ]

    expect(toActionDicts(actionRows)).toEqual([
      {command: FAKE_GESTURE_COMMAND, text_param: 'a'},
      {command: FAKE_GESTURE_COMMAND, text_param: 'b'},
    ])
  })
})
