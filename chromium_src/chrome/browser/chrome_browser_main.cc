/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_main.h"
#include "brave/browser/brave_browser_process_impl.h"

namespace {
#if !defined(OS_ANDROID) && !BUILDFLAG(IS_CHROMEOS_ASH)
void AddFirstRunNewTabs(StartupBrowserCreator* browser_creator,
                        const std::vector<GURL>& new_tabs);
#endif  // !defined(OS_ANDROID) && !BUILDFLAG(IS_CHROMEOS_ASH)
}  // namespace

#define StartupBrowserCreator BraveStartupBrowserCreator
#define BrowserProcessImpl BraveBrowserProcessImpl
#include "../../../../chrome/browser/chrome_browser_main.cc"
#undef BrowserProcessImpl
#undef StartupBrowserCreator

namespace {
#if !defined(OS_ANDROID) && !BUILDFLAG(IS_CHROMEOS_ASH)
void AddFirstRunNewTabs(StartupBrowserCreator* browser_creator,
                        const std::vector<GURL>& new_tabs) {
  AddFirstRunNewTabs(
      reinterpret_cast<BraveStartupBrowserCreator*>(browser_creator), new_tabs);
}
#endif  // !defined(OS_ANDROID) && !BUILDFLAG(IS_CHROMEOS_ASH)
}  // namespace
