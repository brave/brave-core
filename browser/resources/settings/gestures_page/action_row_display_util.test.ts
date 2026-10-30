/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  hasTextParam, primaryActionRow, chainedActionRows, hasChainedActions,
  isActionIncomplete, getCommandSelectClass, getUrlAttachClass,
  optionMatchesCommand, getUrlButtonLabel
} from './action_row_display_util'
import {GestureCommand} from './gesture_command'
import {FAKE_GESTURE_COMMAND} from './test/gesture_test_util'
import type {ActionRow} from './gesture_rows'

const i18n = (key: string) => key

describe('hasTextParam', () => {
  test('should return true for Navigate to URL and Switch to Tab', () => {
    expect(hasTextParam(GestureCommand.NAVIGATE_TO_URL)).toBe(true)
    expect(hasTextParam(GestureCommand.SWITCH_TO_TAB)).toBe(true)
  })

  test('should return false for commands without text parameters', () => {
    expect(hasTextParam(FAKE_GESTURE_COMMAND)).toBe(false)
  })
})

describe('primaryActionRow/chainedActionRows/hasChainedActions', () => {
  const primary: ActionRow =
    {command: 1, textParam: '', openSettingsPage: 0}
  const chained: ActionRow =
    {command: 2, textParam: '', openSettingsPage: 0}

  test('should split the primary row from any chained rows', () => {
    expect(primaryActionRow([primary, chained])).toBe(primary)
    expect(chainedActionRows([primary, chained])).toEqual([chained])
  })

  test('should report no chained actions when only a primary row exists', () => {
    expect(hasChainedActions([primary])).toBe(false)
  })

  test('should report chained actions when more than a primary row exists', () => {
    expect(hasChainedActions([primary, chained])).toBe(true)
  })
})

describe('isActionIncomplete', () => {
  test('should return true when the action is unassigned', () => {
    expect(isActionIncomplete(GestureCommand.UNASSIGNED, '')).toBe(true)
  })

  test('should return true when a text-parameter command has no value', () => {
    expect(isActionIncomplete(GestureCommand.NAVIGATE_TO_URL, '')).toBe(true)
  })

  test('should return false when a text-parameter command has a value', () => {
    expect(isActionIncomplete(
      GestureCommand.NAVIGATE_TO_URL, 'https://example.com')).toBe(false)
  })

  test('should return false for other configured commands', () => {
    expect(isActionIncomplete(FAKE_GESTURE_COMMAND, '')).toBe(false)
  })
})

describe('getCommandSelectClass', () => {
  test('should flag an unassigned command', () => {
    expect(getCommandSelectClass(GestureCommand.UNASSIGNED, ''))
      .toBe('md-select command-unassigned action-incomplete')
  })

  test('should flag an incomplete text param command', () => {
    expect(getCommandSelectClass(GestureCommand.NAVIGATE_TO_URL, ''))
      .toBe('md-select action-incomplete')
  })

  test('should return no extra classes for a complete command', () => {
    expect(getCommandSelectClass(FAKE_GESTURE_COMMAND, '')).toBe('md-select')
  })
})

describe('getUrlAttachClass', () => {
  test('should flag an empty URL or pattern as incomplete', () => {
    expect(getUrlAttachClass('')).toBe('action-param param-attach action-incomplete')
  })

  test('should not flag a populated URL or pattern as incomplete', () => {
    expect(getUrlAttachClass('https://example.com')).toBe('action-param param-attach')
  })
})

describe('optionMatchesCommand', () => {
  test('should match equal values', () => {
    expect(optionMatchesCommand(FAKE_GESTURE_COMMAND, FAKE_GESTURE_COMMAND))
      .toBe(true)
  })

  test('should not match different values', () => {
    expect(optionMatchesCommand(FAKE_GESTURE_COMMAND, GestureCommand.UNASSIGNED))
      .toBe(false)
  })
})

describe('getUrlButtonLabel', () => {
  test('should prompt to set a URL when empty', () => {
    expect(getUrlButtonLabel(GestureCommand.NAVIGATE_TO_URL, '', i18n))
      .toBe('braveGesturesSetUrl')
  })

  test('should prompt to set a pattern for Switch to Tab when empty', () => {
    expect(getUrlButtonLabel(GestureCommand.SWITCH_TO_TAB, '', i18n))
      .toBe('braveGesturesSetPattern')
  })

  test('should strip the scheme from a stored URL', () => {
    expect(getUrlButtonLabel(
      GestureCommand.NAVIGATE_TO_URL, 'https://example.com/', i18n))
      .toBe('example.com/')
  })

  test('should show a stored pattern as-is for Switch to Tab', () => {
    expect(getUrlButtonLabel(GestureCommand.SWITCH_TO_TAB, '*title*', i18n))
      .toBe('*title*')
  })
})
