/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Include all headers in chrome/browser/themes/theme_service_win.h to
// prevent applying newly defined DISALLOW_COPY_AND_ASSIGN macro.
// If not, many static_assert are catched.
#include "base/optional.h"
#include "base/win/registry.h"
#include "chrome/browser/themes/theme_service.h"

#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/brave_override_util.h"

#undef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                                    \
  DISALLOW_COPY(TypeName);                                                                    \
  DISALLOW_ASSIGN(TypeName);                                                                  \
  static_assert(strings_equal(#TypeName, "ThemeServiceWin"), "Use only for ThemeServiceWin"); \
  friend class BraveThemeServiceWin

#undef ThemeService
#define ThemeService BraveThemeService

#include "../../../../../chrome/browser/themes/theme_service_win.h"

#undef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  DISALLOW_COPY(TypeName);                 \
  DISALLOW_ASSIGN(TypeName)

#undef ThemeService
#define ThemeService ThemeService
