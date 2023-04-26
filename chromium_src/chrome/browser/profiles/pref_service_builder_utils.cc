/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/pref_service_builder_utils.h"

#include "brave/browser/brave_profile_prefs.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/common/pref_names.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/spellcheck/browser/pref_names.h"
#include "ui/color/system_theme.h"

#define RegisterProfilePrefs RegisterProfilePrefs_ChromiumImpl
#include "src/chrome/browser/profiles/pref_service_builder_utils.cc"
#undef RegisterProfilePrefs

// Prefs for KeyedService
void RegisterProfilePrefs(bool is_signin_profile,
                          const std::string& locale,
                          user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefs_ChromiumImpl(is_signin_profile, locale, registry);

  brave_ads::AdsService::RegisterProfilePrefs(registry);
  brave_rewards::RewardsService::RegisterProfilePrefs(registry);

  // Disable spell check service
  registry->SetDefaultPrefValue(
      spellcheck::prefs::kSpellCheckUseSpellingService, base::Value(false));

  registry->SetDefaultPrefValue(prefs::kSigninAllowedOnNextStartup,
                                base::Value(false));
#if BUILDFLAG(IS_LINUX)
  // Use brave theme by default instead of gtk theme.
  registry->SetDefaultPrefValue(prefs::kUsesSystemThemeDeprecated,
                                base::Value(false));
  registry->SetDefaultPrefValue(
      prefs::kSystemTheme,
      base::Value(static_cast<int>(ui::SystemTheme::kDefault)));
#endif
}
