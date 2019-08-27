/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_utils.h"

#include "ui/native_theme/native_theme.h"

#if defined(OS_WIN)
#include "ui/native_theme/native_theme_win.h"
#endif

namespace ui {
// static
void BraveThemeUtils::SetDarkMode(bool dark_mode) {
  NativeTheme::GetInstanceForNativeUi()->set_dark_mode(dark_mode);
}

// static
void BraveThemeUtils::SetPreferredColorScheme(BraveThemeType brave_theme_type) {
  NativeTheme::PreferredColorScheme preferred_color_scheme =
      NativeTheme::PreferredColorScheme::kNoPreference;
  switch (brave_theme_type) {
    case BraveThemeType::BRAVE_THEME_TYPE_DEFAULT:
      preferred_color_scheme = NativeTheme::GetInstanceForNativeUi()
                                   ->CalculatePreferredColorScheme();
      break;
    case BraveThemeType::BRAVE_THEME_TYPE_DARK:
      preferred_color_scheme = NativeTheme::PreferredColorScheme::kDark;
      break;
    case BraveThemeType::BRAVE_THEME_TYPE_LIGHT:
      preferred_color_scheme = NativeTheme::PreferredColorScheme::kLight;
      break;
    default:
      NOTREACHED();
  }
  NativeTheme::GetInstanceForNativeUi()->set_preferred_color_scheme(
      preferred_color_scheme);
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
    DCHECK(SystemThemeSupportDarkMode());
    // This sets preferred color scheme on its own.
    ui::UpdateDarkModeStatus();
    return;
#else
    // Linux doesn't support system dark theme so there is no chance to set
    // default type. Default is used for 'Same as Windows/MacOS'.
    NOTREACHED();
#endif
  }

  ui::BraveThemeUtils::SetDarkMode(type ==
                                   BraveThemeType::BRAVE_THEME_TYPE_DARK);
  ui::BraveThemeUtils::SetPreferredColorScheme(type);
  // Have to notify observers explicitly because
  // |ui::BraveThemeUtils::SetDarkMode()| and
  // |ui::BraveThemeUtils::SetPreferredColorScheme| just set
  // ui::NativeTheme:dark_mode_ and ui::NativeTheme:preferred_color_scheme_
  // values.
  ui::NativeTheme::GetInstanceForNativeUi()->NotifyObservers();
}
#endif

#if defined(OS_LINUX)
bool SystemThemeSupportDarkMode() {
  // Linux doesn't support dark mode yet.
  return false;
}
#endif

ui::NativeTheme::PreferredColorScheme GetBravePreferredColorScheme(
    const ui::NativeTheme* native_theme,
    Profile* profile) {
  BraveThemeType brave_theme_type =
      BraveThemeService::GetActiveBraveThemeType(profile);
  switch (brave_theme_type) {
    case BraveThemeType::BRAVE_THEME_TYPE_DEFAULT:
      return native_theme->GetPreferredColorScheme();
    case BraveThemeType::BRAVE_THEME_TYPE_DARK:
      return ui::NativeTheme::PreferredColorScheme::kDark;
    case BraveThemeType::BRAVE_THEME_TYPE_LIGHT:
      return ui::NativeTheme::PreferredColorScheme::kLight;
    default:
      NOTREACHED();
  }
  return ui::NativeTheme::PreferredColorScheme::kNoPreference;
}
