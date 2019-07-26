/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/pref_service_builder_utils.h"

#define RegisterProfilePrefs RegisterProfilePrefs_ChromiumImpl
#include "../../../../../chrome/browser/profiles/pref_service_builder_utils.cc"
#undef RegisterProfilePrefs

#include "brave/browser/brave_profile_prefs.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/common/pref_names.h"
#include "components/spellcheck/browser/pref_names.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service.h"
#endif

// Prefs for KeyedService
void RegisterProfilePrefs(bool is_signin_profile,
                          const std::string& locale,
                          user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefs_ChromiumImpl(is_signin_profile, locale, registry);

  // appearance
#if !defined(OS_ANDROID)
  BraveThemeService::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_TOR)
  tor::TorProfileService::RegisterProfilePrefs(registry);
#endif

  brave_rewards::RewardsService::RegisterProfilePrefs(registry);

  // Disable spell check service
  registry->SetDefaultPrefValue(
      spellcheck::prefs::kSpellCheckUseSpellingService, base::Value(false));

  // Make sure sign into Brave is not enabled
  // The older kSigninAllowed is deprecated and only in use in Android until
  // C71.
  registry->SetDefaultPrefValue(prefs::kSigninAllowedOnNextStartup,
                                base::Value(false));

#if defined(OS_LINUX)
  // Use brave theme by default instead of gtk theme.
  registry->SetDefaultPrefValue(prefs::kUsesSystemTheme, base::Value(false));
#endif
}
