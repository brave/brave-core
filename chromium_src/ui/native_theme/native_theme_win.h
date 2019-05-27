/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../ui/native_theme/native_theme_win.h"

// Custom header guard is used to avoid including function declaration multiple
// times.
#ifndef UI_NATIVE_THEME_NATIVE_THEME_WIN_H_BRAVE_  // NOLINT
#define UI_NATIVE_THEME_NATIVE_THEME_WIN_H_BRAVE_  // NOLINT

namespace ui {

// If |override| is true, system dark mode is overridden by |enable_dark_mode|.
// If |override| is false, |enable_dark_mode| is ignored.
void NATIVE_THEME_EXPORT SetOverrideSystemDarkMode(bool override,
                                                   bool enable_dark_mode);
}  // namespace ui

#endif  // UI_NATIVE_THEME_NATIVE_THEME_WIN_H_BRAVE_  // NOLINT
