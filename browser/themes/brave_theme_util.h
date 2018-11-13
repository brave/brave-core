/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_UTIL_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_UTIL_H_

#include "brave/browser/themes/brave_theme_service.h"

namespace brave {

// Return true when platform has theme type and |type| stores platform's theme
// type. Otherwise returns false.
bool GetPlatformThemeType(BraveThemeType* type);

}  // namespace brave

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_UTIL_H_
