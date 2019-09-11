/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_UTILS_INTERNAL_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_UTILS_INTERNAL_H_

#include "brave/browser/themes/brave_theme_service.h"

namespace internal {

// Reset dark mode and preferred color scheme and notify.
void SetSystemThemeForNonDarkModePlatform(BraveThemeType type);

}  // namespace internal

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_UTILS_INTERNAL_H_
