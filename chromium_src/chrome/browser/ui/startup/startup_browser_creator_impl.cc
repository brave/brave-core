/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "chrome/browser/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "components/signin/public/base/signin_buildflags.h"

#if BUILDFLAG(ENABLE_UPDATER)
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/cocoa/keystone_infobar_delegate.h"
#include "chrome/browser/ui/startup/startup_types.h"
#endif

#if BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_UPDATER)
namespace {
void MaybeShowPromotionInfoBar(
    chrome::startup::IsProcessStartup process_startup) {
  if (process_startup == chrome::startup::IsProcessStartup::kYes) {
    // Check whether the auto-update system needs to be promoted from user
    // to system.
    ShowUpdaterPromotionInfoBar();
  }
}
}  // namespace
#endif

// This override is in place to make sure the first run page from Chromium is
// not shown, as Brave has its own first run page
#define BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINESTARTUPTABS            \
  has_first_run_experience = false;                                     \
  if (is_first_run_ == chrome::startup::IsFirstRun::kYes) {             \
    StartupTabs onboarding_tabs = provider.GetOnboardingTabs(profile_); \
    AppendTabs(onboarding_tabs, &tabs);                                 \
    has_first_run_experience = !onboarding_tabs.empty();                \
  }

#if BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_UPDATER)
#define GetLastActive                         \
  GetLastActive();                            \
  MaybeShowPromotionInfoBar(process_startup); \
  BrowserList::GetInstance()->GetLastActive
#endif

#include "src/chrome/browser/ui/startup/startup_browser_creator_impl.cc"

#if BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_UPDATER)
#undef GetLastActive
#endif

#undef BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINESTARTUPTABS
