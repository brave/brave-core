/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_UTILS_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_UTILS_H_

#include "brave/browser/themes/brave_theme_service.h"

bool SystemThemeSupportDarkMode();

// When system supports system per-application system theme changing, set it.
// Currently, only MacOS support it.
// Otherewise, we need to overrides from native theme level and explicitly
// notifying to let observers know.
// By overriding, base ui components also use same brave theme type.
void SetSystemTheme(BraveThemeType type);

// Inserted in the ui namespace to add into ui::NativeTheme/NativeThemeWin as a
// friend function. This function calls protected/private method of
// ui::NativeTheme::set_dark_mode(). It's a protected method that called by
// platform specific subclasses whenever system os theme is changed.
// But we want to change it for using brave theme also for webui/base ui
// modules like context menu.
namespace ui {
void SetDarkMode(bool dark_mode);
}  // namespace ui

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_UTILS_H_
