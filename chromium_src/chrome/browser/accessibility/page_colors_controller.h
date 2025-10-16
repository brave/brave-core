// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ACCESSIBILITY_PAGE_COLORS_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ACCESSIBILITY_PAGE_COLORS_CONTROLLER_H_

// This replacement has been made necessary because
// PageColorsController::MigrateObsoleteProfilePrefs gets called from
// MigrateObsoleteProfilePrefs in chrome/browser/prefs/browser_prefs.cc by
// another local MigrateObsoleteProfilePrefs. That function is replaced in Brave
// with our overrides with MigrateObsoleteProfilePrefs_ChromiumImpl, and that
// causes a substitution for the callsite of
// PageColorsController::MigrateObsoleteProfilePrefs as too. This substitution
// is then being introduced to avoid any upstream patching for this.
#define MigrateObsoleteProfilePrefs MigrateObsoleteProfilePrefs_ChromiumImpl
#include <chrome/browser/accessibility/page_colors_controller.h>  // IWYU pragma: export
#undef MigrateObsoleteProfilePrefs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ACCESSIBILITY_PAGE_COLORS_CONTROLLER_H_
