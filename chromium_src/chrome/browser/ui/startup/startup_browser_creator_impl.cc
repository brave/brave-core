/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile.h"
#include "components/signin/public/base/signin_buildflags.h"

// This override is in place to make sure the first run page from Chromium is
// not shown, as Brave has its own first run page
#define BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINESTARTUPTABS            \
  has_first_run_experience = false;                                     \
  if (is_first_run_ == chrome::startup::IsFirstRun::kYes) {             \
    StartupTabs onboarding_tabs = provider.GetOnboardingTabs(profile_); \
    AppendTabs(onboarding_tabs, &tabs);                                 \
    has_first_run_experience = !onboarding_tabs.empty();                \
  }

#include "src/chrome/browser/ui/startup/startup_browser_creator_impl.cc"
#undef BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINESTARTUPTABS
