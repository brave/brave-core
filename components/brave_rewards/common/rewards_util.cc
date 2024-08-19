/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/rewards_util.h"

#include <string>

#include "base/no_destructor.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/ofac_sanction_util.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/feature_list.h"
#include "brave/components/brave_rewards/common/features.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "brave/components/brave_rewards/common/pref_names.h"
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

namespace brave_rewards {

namespace {

bool IsDisabledByPolicy(PrefService* prefs) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  DCHECK(prefs);
  return prefs->IsManagedPreference(prefs::kDisabledByPolicy) &&
         prefs->GetBoolean(prefs::kDisabledByPolicy);
#else
  return false;
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
}

bool IsDisabledByFeature() {
#if BUILDFLAG(IS_ANDROID)
  if (!base::FeatureList::IsEnabled(features::kBraveRewards)) {
    return true;
  }
#endif  // BUILDFLAG(IS_ANDROID)
  return false;
}

bool IsOFACSanctionedRegion(const std::string& country_code) {
  return brave_l10n::IsISOCountryCodeOFACSanctioned(country_code);
}

const std::string GetCountryCode() {
  return brave_l10n::GetDefaultISOCountryCodeString();
}

}  // namespace

bool IsSupported(PrefService* prefs, IsSupportedOptions options) {
  bool is_supported = !IsDisabledByPolicy(prefs) && !IsDisabledByFeature();
  if (is_supported && options != IsSupportedOptions::kSkipRegionCheck) {
    return !IsUnsupportedRegion();
  }
  return is_supported;
}

bool IsUnsupportedRegion() {
  return IsOFACSanctionedRegion(GetCountryCode());
}

bool IsAutoContributeSupportedForCountry(std::string_view country_code) {
  return country_code != "JP" && country_code != "IN";
}

}  // namespace brave_rewards
