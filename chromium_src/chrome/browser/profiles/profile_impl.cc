/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_impl.h"

#include "brave/browser/brave_browser_features.h"

#define ShouldRestoreOldSessionCookies \
  ShouldRestoreOldSessionCookies_ChromiumImpl

#include "src/chrome/browser/profiles/profile_impl.cc"

#undef ShouldRestoreOldSessionCookies

bool ProfileImpl::ShouldRestoreOldSessionCookies() {
  bool should_restore = ShouldRestoreOldSessionCookies_ChromiumImpl();
  if (base::FeatureList::IsEnabled(
          features::kBraveCleanupSessionCookiesOnSessionRestore)) {
#if BUILDFLAG(IS_ANDROID)
    should_restore = false;
#else   // !BUILDFLAG(IS_ANDROID)
    if (ExitTypeService::GetLastSessionExitType(this) != ExitType::kCrashed) {
      should_restore &= StartupBrowserCreator::WasRestarted();
    }
#endif  // BUILDFLAG(IS_ANDROID)
  }
  return should_restore;
}
