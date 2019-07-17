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
void SetDarkMode(bool dark_mode) {
    ui::NativeTheme::GetInstanceForNativeUi()->set_dark_mode(dark_mode);
}  // namespace ui

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
    ui::UpdateDarkModeStatus();
    return;
#else
    // Linux doesn't support system dark theme so there is no chance to set
    // default type. Default is used for 'Same as Windows/MacOS'.
    NOTREACHED();
#endif
  }

  ui::SetDarkMode(type == BraveThemeType::BRAVE_THEME_TYPE_DARK);
  // Have to notify to observer explicitly because |ui::SetDarkMode()| just
  // set ui::NativeTheme:dark_mode_ value.
  ui::NativeTheme::GetInstanceForNativeUi()->NotifyObservers();
}
#endif

#if defined(OS_LINUX)
bool SystemThemeSupportDarkMode() {
  // Linux doesn't support dark mode yet.
  return false;
}
#endif
