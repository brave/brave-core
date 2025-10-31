/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_H_
#define BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_H_

class PrefRegistrySimple;

namespace dark_mode {

// Don't use this enum anymore. It's deprecated and only used for migration.
enum class BraveDarkModeType {
  // DEFAULT type acts as two ways depends on system theme mode.
  // If system dark mode is not supported, we override it with channel based
  // policy. In this case, user can see dark or light option in settings.
  // Otherwise, it acts like system dark mode mode. It respects system's dark
  // mode. In this case, user can see all three options in theme settings.
  BRAVE_DARK_MODE_TYPE_DEFAULT,
  BRAVE_DARK_MODE_TYPE_DARK,
  BRAVE_DARK_MODE_TYPE_LIGHT,
};

// APIs for prefs.
void RegisterBraveDarkModeLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace dark_mode

#endif  // BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_H_
