/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_helper_win.h"

#include "chrome/browser/themes/theme_properties.h"

SkColor BraveThemeHelperWin::GetDefaultColor(
    int id,
    bool incognito,
    const CustomThemeSupplier* theme_supplier) const {
  // Prevent dcheck in chrome/browser/themes/theme_properties.cc(384)
  // It assumes these ids are handled in theme service.
  if (id == ThemeProperties::COLOR_ACCENT_BORDER_ACTIVE ||
      id == ThemeProperties::COLOR_ACCENT_BORDER_INACTIVE) {
    return ThemeHelperWin::GetDefaultColor(id, incognito, theme_supplier);
  }
  // Skip ThemeHelperWin::GetDefaultColor() to prevent using dwm frame color.
  return BraveThemeHelper::GetDefaultColor(id, incognito, theme_supplier);
}
