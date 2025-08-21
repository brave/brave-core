/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_MAIN_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_MAIN_H_

#include "build/build_config.h"

#define ChromeBrowserMainParts ChromeBrowserMainParts_ChromiumImpl
#include <chrome/browser/chrome_browser_main.h>  // IWYU pragma: export
#undef ChromeBrowserMainParts

namespace base {
class CommandLine;
}

class Profile;

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)
// Processes theme command line switches for the specified profile.
// Desktop platforms only (Windows, macOS, Linux, ChromeOS).
void ProcessThemeCommandLineSwitchesForProfile(
    const base::CommandLine* command_line,
    Profile* profile);
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) ||
        // BUILDFLAG(IS_CHROMEOS)

class ChromeBrowserMainParts : public ChromeBrowserMainParts_ChromiumImpl {
 public:
  ChromeBrowserMainParts(bool is_integration_test, StartupData* startup_data);

  ChromeBrowserMainParts(const ChromeBrowserMainParts&) = delete;
  ChromeBrowserMainParts& operator=(const ChromeBrowserMainParts&) = delete;
  ~ChromeBrowserMainParts() override;

  int PreMainMessageLoopRun() override;
  void PreBrowserStart() override;
  void PostBrowserStart() override;
  void PreShutdown() override;
  void PreProfileInit() override;
  void PostProfileInit(Profile* profile, bool is_initial_profile) override;

 private:
  friend class ChromeBrowserMainExtraPartsTor;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_MAIN_H_
