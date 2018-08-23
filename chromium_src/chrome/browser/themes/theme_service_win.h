/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#undef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  DISALLOW_COPY(TypeName);                 \
  DISALLOW_ASSIGN(TypeName);               \
  friend class BraveThemeServiceWin

#undef ThemeService
#define ThemeService BraveThemeService

#include "../../../../../chrome/browser/themes/theme_service_win.h"

#undef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  DISALLOW_COPY(TypeName);                 \
  DISALLOW_ASSIGN(TypeName)

#undef ThemeService
