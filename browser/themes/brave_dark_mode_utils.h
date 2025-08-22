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

namespace base {
class CommandLine;
}

class Profile;

// Processes browser-wide theme command line switches.
// This should be called once during browser startup.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)
void ProcessBrowserWideThemeCommandLineSwitches();
void ProcessBrowserWideThemeCommandLineSwitches(
    const base::CommandLine* command_line);
void ProcessBrowserWideThemeCommandLineSwitches(
    const base::CommandLine* command_line,
    Profile* single_profile);
#endif

// Processes per-profile theme command line switches for the specified profile.
// Desktop platforms only (Windows, macOS, Linux, ChromeOS).
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)
void ProcessThemeCommandLineSwitchesForProfile(
    const base::CommandLine* command_line,
    Profile* profile);
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) ||
        // BUILDFLAG(IS_CHROMEOS)

#endif  // BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_H_
