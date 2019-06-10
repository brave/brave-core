/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_utils.h"

bool SystemThemeSupportDarkMode() {
  // Linux doesn't support dark mode yet.
  return false;
}

void SetSystemTheme(BraveThemeType type) {}
