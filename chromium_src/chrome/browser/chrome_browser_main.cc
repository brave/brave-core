/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_main.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"

// Macro injected into ProcessSingletonNotificationCallbackImpl to handle
// theme switches when Chrome is already running and receives new command line args.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
#define BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL \
  ProfileManager* profile_manager = g_browser_process->profile_manager(); \
  if (profile_manager) { \
    Profile* profile = \
        profile_manager->GetProfileByPath(startup_profile_path_info.path); \
    if (profile) { \
      ProcessThemeCommandLineSwitchesForProfile(&command_line, profile); \
    } \
  }
#else
#define BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL
#endif  // #if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

// Macro injected into PostProfileInit to handle theme switches during
// initial profile setup after Chrome startup.
#if !BUILDFLAG(IS_ANDROID)
#define BRAVE_POST_PROFILE_INIT \
  ProcessThemeCommandLineSwitchesForProfile( \
      base::CommandLine::ForCurrentProcess(), profile);
#else
#define BRAVE_POST_PROFILE_INIT
#endif  // !BUILDFLAG(IS_ANDROID)

#define BrowserProcessImpl BraveBrowserProcessImpl
#define ChromeBrowserMainParts ChromeBrowserMainParts_ChromiumImpl
#include <chrome/browser/chrome_browser_main.cc>
#undef ChromeBrowserMainParts
#undef BrowserProcessImpl
#undef BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL
#undef BRAVE_POST_PROFILE_INIT
