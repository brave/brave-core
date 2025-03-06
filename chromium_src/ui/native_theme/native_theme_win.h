/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_WIN_H_
#define BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_WIN_H_

#define BRAVE_UI_NATIVE_THEME_NATIVE_THEME_WIN_H_ \
 private: \
  friend void UpdateDarkModeStatus();

#include "src/ui/native_theme/native_theme_win.h"  // IWYU pragma: export
#undef BRAVE_UI_NATIVE_THEME_NATIVE_THEME_WIN_H_

namespace ui {

// Set true when browser should not response to system theme change.
// If user set dark or light explicitly, browser should ignore system dark mode
// setting.
void COMPONENT_EXPORT(NATIVE_THEME) IgnoreSystemDarkModeChange(bool ignore);

}  // namespace ui

#endif  // BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_WIN_H_
