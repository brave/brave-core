/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_utils.h"

#include "brave/browser/themes/brave_theme_utils_internal.h"
#include "ui/native_theme/native_theme.h"

#if defined(OS_WIN)
#include "ui/native_theme/native_theme_win.h"
#endif

namespace ui {
// static
void BraveThemeUtils::SetDarkMode(bool dark_mode) {
  NativeTheme::GetInstanceForNativeUi()->set_use_dark_colors(dark_mode);
  NativeTheme::GetInstanceForWeb()->set_use_dark_colors(dark_mode);
}

// static
void BraveThemeUtils::ReCalcAndSetPreferredColorScheme() {
  auto scheme =
      NativeTheme::GetInstanceForNativeUi()->CalculatePreferredColorScheme();
  NativeTheme::GetInstanceForNativeUi()->set_preferred_color_scheme(scheme);
  NativeTheme::GetInstanceForWeb()->set_preferred_color_scheme(scheme);
}

#if defined(OS_WIN)
// This resets dark mode to os theme when user changes brave theme from
// dark or light to Same as Windows by using the value of registry because
// we changed ui::NativeTheme::dark_mode_ explicitly for using brave theme
// like a system theme.
void UpdateDarkModeStatus() {
  static_cast<ui::NativeThemeWin*>(
      ui::NativeTheme::GetInstanceForNativeUi())->UpdateDarkModeStatus();
}
#endif
}  // namespace ui

#if defined(OS_WIN) || defined(OS_LINUX)
void SetSystemTheme(BraveThemeType type) {
  // Follow os theme type for default type.
  if (type == BraveThemeType::BRAVE_THEME_TYPE_DEFAULT) {
#if defined(OS_WIN)
    DCHECK(
        ui::NativeTheme::GetInstanceForNativeUi()->SystemDarkModeSupported());
    // This sets preferred color scheme on its own.
    ui::UpdateDarkModeStatus();
    return;
#else
    // Linux doesn't support system dark theme so there is no chance to set
    // default type. Default is used for 'Same as Windows/MacOS'.
    NOTREACHED();
#endif
  }
  internal::SetSystemThemeForNonDarkModePlatform(type);
}
#endif
