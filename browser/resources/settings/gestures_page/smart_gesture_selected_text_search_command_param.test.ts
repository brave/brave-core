/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  SEARCH_TARGET_I18N_KEYS, SEARCH_TARGET_ORDER,
  SmartGestureSelectedTextSearchCommandParamType,
} from './smart_gesture_selected_text_search_command_param'

describe('SEARCH_TARGET_ORDER', () => {
  test('should list every search target exactly once', () => {
    const allParams = Object.values(SmartGestureSelectedTextSearchCommandParamType)
      .filter((param): param is SmartGestureSelectedTextSearchCommandParamType =>
        typeof param === 'number')

    expect(new Set(SEARCH_TARGET_ORDER).size).toBe(allParams.length)
    expect(SEARCH_TARGET_ORDER.length).toBe(allParams.length)
  })

  test('should list BRAVE_SEARCH before ASK_BRAVE', () => {
    expect(SEARCH_TARGET_ORDER).toEqual([
      SmartGestureSelectedTextSearchCommandParamType.BRAVE_SEARCH,
      SmartGestureSelectedTextSearchCommandParamType.ASK_BRAVE,
    ])
  })
})

describe('SEARCH_TARGET_I18N_KEYS', () => {
  test('should define an i18n key for every search target', () => {
    for (const param of SEARCH_TARGET_ORDER) {
      expect(SEARCH_TARGET_I18N_KEYS[param]).toBeTruthy()
    }
  })

  test('should map BRAVE_SEARCH and ASK_BRAVE to distinct keys', () => {
    expect(SEARCH_TARGET_I18N_KEYS[
      SmartGestureSelectedTextSearchCommandParamType.BRAVE_SEARCH])
      .not.toBe(SEARCH_TARGET_I18N_KEYS[
        SmartGestureSelectedTextSearchCommandParamType.ASK_BRAVE])
  })
})
