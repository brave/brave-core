/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_SERVICE_H_

#define GetOmniboxColor virtual GetOmniboxColor
#define THEMES_THEME_SERVICE_H_ \
  friend class BraveThemeService;

#include "../../../../../chrome/browser/themes/theme_service.h"
#undef THEMES_THEME_SERVICE_H_
#undef GetOmniboxColor

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_SERVICE_H_
