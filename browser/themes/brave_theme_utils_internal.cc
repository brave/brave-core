/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_utils_internal.h"

#include "brave/browser/themes/brave_theme_utils.h"
#include "ui/native_theme/native_theme.h"

namespace internal {

void SetSystemThemeForNonDarkModePlatform(BraveThemeType type) {
  // Call SetDarkMode() first then call ReCalcPreferredColorScheme() because
  // ReCalcPreferredColorScheme() calculates preferred color scheme based on
  // dark mode.
  ui::BraveThemeUtils::SetDarkMode(type ==
                                   BraveThemeType::BRAVE_THEME_TYPE_DARK);
  ui::BraveThemeUtils::ReCalcAndSetPreferredColorScheme();
  // Have to notify observers explicitly because
  // |ui::BraveThemeUtils::SetDarkMode()| and
  // |ui::BraveThemeUtils::ReCalcPreferredColorScheme| just set
  // ui::NativeTheme:dark_mode_ and ui::NativeTheme:preferred_color_scheme_
  // values.
  ui::NativeTheme::GetInstanceForNativeUi()->NotifyObservers();
  ui::NativeTheme::GetInstanceForWeb()->NotifyObservers();
}

}  // namespace internal
