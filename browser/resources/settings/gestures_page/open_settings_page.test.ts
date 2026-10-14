/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  OPEN_SETTINGS_PAGE_ORDER, OPEN_SETTINGS_PAGE_PATHS, OpenSettingsPage,
} from './open_settings_page'

describe('OPEN_SETTINGS_PAGE_ORDER', () => {
  test('should list BASIC first', () => {
    expect(OPEN_SETTINGS_PAGE_ORDER[0]).toBe(OpenSettingsPage.BASIC)
  })

  test('should list every page exactly once', () => {
    const allPages = Object.values(OpenSettingsPage)
      .filter((page): page is OpenSettingsPage => typeof page === 'number')

    expect(new Set(OPEN_SETTINGS_PAGE_ORDER).size).toBe(allPages.length)
    expect(OPEN_SETTINGS_PAGE_ORDER.length).toBe(allPages.length)
  })

  test('should sort every page after BASIC alphabetically by its path', () => {
    const pathsAfterBasic =
      OPEN_SETTINGS_PAGE_ORDER.slice(1).map((page) =>
        OPEN_SETTINGS_PAGE_PATHS[page])

    expect(pathsAfterBasic).toEqual([...pathsAfterBasic].sort())
  })
})
