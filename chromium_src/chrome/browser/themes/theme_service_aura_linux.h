/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#undef ThemeService
#define ThemeService BraveThemeService
#include "../../../../../chrome/browser/themes/theme_service_aura_linux.h"
#undef ThemeService
