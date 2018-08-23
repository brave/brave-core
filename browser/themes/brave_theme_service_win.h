/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_WIN_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_WIN_H_

#include "chrome/browser/themes/theme_service_win.h"

class BraveThemeServiceWin : public ThemeServiceWin {
 public:
  BraveThemeServiceWin() = default;
  ~BraveThemeServiceWin() override = default;

 private:
  // ThemeServiceWin overrides:
  SkColor GetDefaultColor(int id, bool incognito) const override;

  DISALLOW_COPY_AND_ASSIGN(BraveThemeServiceWin);
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_WIN_H_
