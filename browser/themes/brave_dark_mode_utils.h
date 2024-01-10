/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_H_
#define BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_H_

#include <string>

#include "base/values.h"
#include "build/build_config.h"

class PrefRegistrySimple;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace dark_mode {

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
void RegisterBraveDarkModePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry);
void RegisterBraveDarkModeLocalStatePrefs(PrefRegistrySimple* registry);

std::string GetStringFromBraveDarkModeType(BraveDarkModeType type);
base::Value::List GetBraveDarkModeTypeList();
void SetBraveDarkModeType(const std::string& type);
void SetBraveDarkModeType(BraveDarkModeType type);
// Returns current effective theme type. dark or light.
BraveDarkModeType GetActiveBraveDarkModeType();
// Returns current theme type.
// dark/light will be returned if platform doesn't support system dark mode.
// Otherwise, returns default/dark/light.
BraveDarkModeType GetBraveDarkModeType();
bool SystemDarkModeEnabled();
void SetUseSystemDarkModeEnabledForTest(bool enabled);

// When system supports system per-application system theme changing, set it.
// Currently, only MacOS support it.
// Otherewise, we need to overrides from native theme level and explicitly
// notifying to let observers know.
// By overriding, base ui components also use same brave theme type.
void SetSystemDarkMode(BraveDarkModeType type);

#if BUILDFLAG(IS_LINUX)
// Cache system preference from DarkModeManagerLinux.
// This cached value is used whenever user chooses "Same as Linux" option.
void CacheSystemDarkModePrefs(bool prefer_dark_theme);
bool HasCachedSystemDarkModeType();
#endif

}  // namespace dark_mode

#endif  // BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_H_
