/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_main.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "chrome/browser/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"

// Macro injected into ProcessSingletonNotificationCallbackImpl to handle
// theme switches when Chrome is already running and receives new command line
// args.
#if BUILDFLAG(ENABLE_PROCESS_SINGLETON)
#define BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL                 \
  dark_mode::ProcessBrowserWideThemeCommandLineSwitches(&command_line);    \
  ProfileManager* profile_manager = g_browser_process->profile_manager();  \
  if (profile_manager) {                                                   \
    Profile* profile =                                                     \
        profile_manager->GetProfileByPath(startup_profile_path_info.path); \
    if (profile) {                                                         \
      dark_mode::ProcessThemeCommandLineSwitchesForProfile(&command_line,  \
                                                           profile);       \
    }                                                                      \
  }
#endif  // BUILDFLAG(ENABLE_PROCESS_SINGLETON)

#define BrowserProcessImpl BraveBrowserProcessImpl
#define ChromeBrowserMainParts ChromeBrowserMainParts_ChromiumImpl
#include <chrome/browser/chrome_browser_main.cc>
#undef ChromeBrowserMainParts
#undef BrowserProcessImpl
#undef BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL
