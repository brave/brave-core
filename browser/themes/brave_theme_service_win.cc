/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service_win.h"

#include "chrome/browser/themes/theme_properties.h"

SkColor BraveThemeServiceWin::GetDefaultColor(int id, bool incognito) const {
  // Prevent dcheck in chrome/browser/themes/theme_properties.cc(384)
  // It assumes this id handled in theme service.
  if (DwmColorsAllowed() && id == ThemeProperties::COLOR_ACCENT_BORDER)
    return dwm_accent_border_color_;
  // Skip ThemeServiceWin::GetDefaultColor() to prevent using dwm frame color.
  return BraveThemeService::GetDefaultColor(id, incognito);
}
