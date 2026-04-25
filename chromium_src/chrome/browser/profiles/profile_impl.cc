/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_impl.h"

#include "brave/browser/brave_browser_features.h"
#include "build/build_config.h"

#define ShouldRestoreOldSessionCookies \
  ShouldRestoreOldSessionCookies_ChromiumImpl

#if BUILDFLAG(IS_ANDROID)
namespace brave_policy {
void SetBraveProfilePolicyProviderProfileID(
    policy::ConfigurationPolicyProvider* provider,
    const base::FilePath& profile_path);
}

#define BRAVE_PROFILE_IMPL_TAKE_PREFS_FROM_STARTUP_DATA                 \
  brave_policy::SetBraveProfilePolicyProviderProfileID(                 \
      profile_policy_connector_->GetBraveProfilePolicyProvider().get(), \
      GetPath());
#endif  // BUILDFLAG(IS_ANDROID)

#include <chrome/browser/profiles/profile_impl.cc>

#undef ShouldRestoreOldSessionCookies
#if BUILDFLAG(IS_ANDROID)
#undef BRAVE_PROFILE_IMPL_TAKE_PREFS_FROM_STARTUP_DATA
#endif  // BUILDFLAG(IS_ANDROID)

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
