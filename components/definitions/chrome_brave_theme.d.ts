// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

declare namespace chrome.braveTheme {
  type ThemeType = 'Light' | 'Dark' | 'System'
  type ThemeItem = {name: ThemeType, index: number}
  type ThemeList = ThemeItem[] // For backwards compatibility, but can be removed
  type ThemeTypeCallback = (themeType: ThemeType) => unknown
  type ThemeListCallback = (themeList: string /* JSON of type ThemeItem[] */) => unknown
  const getBraveThemeType: (themeType: ThemeTypeCallback) => void
  const getBraveThemeList: (cb: ThemeListCallback) => void
  const setBraveThemeType: (themeType: ThemeType) => void
  const onBraveThemeTypeChanged: {
    addListener: (callback: ThemeTypeCallback) => void
  }
}
