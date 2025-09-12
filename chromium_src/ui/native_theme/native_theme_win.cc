/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

namespace {
bool s_ignore_system_dark_mode_change = false;
}  // namespace

#define BRAVE_NATIVETHEMEWIN_UPDATEDARKMODESTATUS \
  if (s_ignore_system_dark_mode_change) {         \
    return;                                       \
  }

#include <ui/native_theme/native_theme_win.cc>
#undef BRAVE_NATIVETHEMEWIN_UPDATEDARKMODESTATUS

namespace ui {

// Shared instance for dark UI. This was part of Chromium, but got removed in
// Chromium 141. However, we use it for Private/Tor windows.
NativeTheme* NativeTheme::GetInstanceForDarkUI() {
  static base::NoDestructor<NativeThemeWin> s_dark_native_theme;
  return s_dark_native_theme.get();
}

void COMPONENT_EXPORT(NATIVE_THEME) IgnoreSystemDarkModeChange(bool ignore) {
  s_ignore_system_dark_mode_change = ignore;
}

}  // namespace ui
