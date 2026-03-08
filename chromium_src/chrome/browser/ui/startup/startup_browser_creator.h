/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_STARTUP_BROWSER_CREATOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_STARTUP_BROWSER_CREATOR_H_

// Adds _ChromiumImpl method declarations alongside the originals, so the Brave
// Origin startup dialog can intercept the startup flow in the .cc override.
// Start() is only called from chrome_browser_main.cc (a different translation
// unit) so the #define in the .cc does not affect its external callers.
#define Start                                                                 \
  Start_ChromiumImpl(                                                         \
      const base::CommandLine& cmd_line, const base::FilePath& cur_dir,       \
      StartupProfileInfo profile_info, const Profiles& last_opened_profiles); \
  bool Start

#define ProcessCommandLineAlreadyRunning                                    \
  ProcessCommandLineAlreadyRunning_ChromiumImpl(                            \
      const base::CommandLine& command_line, const base::FilePath& cur_dir, \
      const StartupProfilePathInfo& profile_path_info);                     \
  static void ProcessCommandLineAlreadyRunning

#include <chrome/browser/ui/startup/startup_browser_creator.h>  // IWYU pragma: export
#undef ProcessCommandLineAlreadyRunning
#undef Start

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_STARTUP_BROWSER_CREATOR_H_
