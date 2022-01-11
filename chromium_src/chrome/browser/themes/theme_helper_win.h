/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_HELPER_WIN_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_HELPER_WIN_H_

#include "brave/browser/themes/brave_theme_helper.h"

#undef ThemeHelper
#define ThemeHelper BraveThemeHelper

#define BRAVE_THEME_HELPER_WIN_H_   \
 private:                           \
  friend class BraveThemeHelperWin; \
                                    \
 public:
#include "src/chrome/browser/themes/theme_helper_win.h"

#undef BRAVE_THEME_HELPER_WIN_H_
#undef ThemeHelper
#define ThemeHelper ThemeHelper

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_HELPER_WIN_H_
