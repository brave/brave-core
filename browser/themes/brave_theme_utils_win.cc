/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_utils.h"

#include <windows.h>

#include "base/win/registry.h"

// Copied from ctor of NativeThemeWin.
bool SystemThemeSupportDarkMode() {
  // Dark Mode currently targets UWP apps, which means Win32 apps need to use
  // alternate, less reliable means of detecting the state. The following
  // can break in future Windows versions.
  base::win::RegKey hkcu_themes_regkey;
  bool key_open_succeeded =
      hkcu_themes_regkey.Open(HKEY_CURRENT_USER,
                              L"Software\\Microsoft\\Windows\\CurrentVersion\\"
                              L"Themes\\Personalize",
                              KEY_READ | KEY_NOTIFY) == ERROR_SUCCESS;
  return key_open_succeeded;
}

void SetSystemTheme(BraveThemeType type) {}
