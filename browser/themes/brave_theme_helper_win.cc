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
  // It assumes this id handled in theme service.
  if (DwmColorsAllowed(theme_supplier) &&
      id == ThemeProperties::COLOR_ACCENT_BORDER)
    return dwm_accent_border_color_;
  // Skip ThemeHelperWin::GetDefaultColor() to prevent using dwm frame color.
  return BraveThemeHelper::GetDefaultColor(id, incognito, theme_supplier);
}
