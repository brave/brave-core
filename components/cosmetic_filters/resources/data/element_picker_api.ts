// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Browser-side bridge used by the element picker. Each platform (Chromium,
 * iOS/WebKit) provides an implementation and passes it to
 * `showElementPicker`.
 */
export interface ElementPickerAPI {
  /** Persists a user-created cosmetic filter rule for `selector`. */
  cosmeticFilterCreate: (selector: string) => void
  /** Opens the browser UI for managing custom filters. */
  cosmeticFilterManage: () => void
  /**
   * Fetches the current theme. `bgcolor` is an ARGB integer; only the lower
   * 24 bits (RGB) are used. Called again whenever `prefers-color-scheme`
   * changes.
   */
  getElementPickerThemeInfo: (
    callback: (isDarkModeEnabled: boolean, bgcolor: number) => void,
  ) => void
  /** Fetches localized button labels, in the order of the callback args. */
  getLocalizedTexts: (
    callback: (
      btnCreateDisabledText: string,
      btnCreateEnabledText: string,
      btnManageText: string,
      btnShowRulesBoxText: string,
      btnHideRulesBoxText: string,
      btnQuitText: string,
    ) => void,
  ) => void
  /**
   * Returns the platform name. `'android'` and `'ios'` enable the mobile
   * layout; any other value is treated as desktop.
   */
  getPlatform: () => string
}
