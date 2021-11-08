/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_HELPER_WIN_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_HELPER_WIN_H_

#include "chrome/browser/themes/theme_helper_win.h"

class BraveThemeHelperWin : public ThemeHelperWin {
 public:
  BraveThemeHelperWin() = default;
  BraveThemeHelperWin(const BraveThemeHelperWin&) = delete;
  BraveThemeHelperWin& operator=(const BraveThemeHelperWin&) = delete;
  ~BraveThemeHelperWin() override = default;

 private:
  // ThemeHelperWin overrides:
  SkColor GetDefaultColor(
      int id,
      bool incognito,
      const CustomThemeSupplier* theme_supplier) const override;
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_HELPER_WIN_H_
