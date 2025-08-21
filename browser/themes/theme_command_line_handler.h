/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_THEME_COMMAND_LINE_HANDLER_H_
#define BRAVE_BROWSER_THEMES_THEME_COMMAND_LINE_HANDLER_H_

namespace base {
class CommandLine;
}

class Profile;

namespace brave {
namespace themes {

// Processes theme command line switches for the specified profile.
// Desktop platforms only (Windows, macOS, Linux, ChromeOS).
void ProcessThemeCommandLineSwitchesForProfile(
    const base::CommandLine* command_line,
    Profile* profile);

}  // namespace themes
}  // namespace brave

#endif  // BRAVE_BROWSER_THEMES_THEME_COMMAND_LINE_HANDLER_H_
