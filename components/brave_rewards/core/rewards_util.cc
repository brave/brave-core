/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_util.h"

#include <string>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/l10n/common/ofac_sanction_util.h"
#include "build/build_config.h"
#include "components/country_codes/country_codes.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/feature_list.h"
#include "brave/components/brave_rewards/core/features.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if !BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_rewards/core/pref_names.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace brave_rewards {

namespace {

bool IsDisabledByPolicy(PrefService* prefs) {
#if BUILDFLAG(IS_ANDROID)
  return false;
#else
  DCHECK(prefs);
  return prefs->IsManagedPreference(prefs::kDisabledByPolicy) &&
         prefs->GetBoolean(prefs::kDisabledByPolicy);
#endif
}

bool IsDisabledByFeature() {
#if BUILDFLAG(IS_ANDROID)
  if (!base::FeatureList::IsEnabled(features::kBraveRewards)) {
    return true;
  }
#endif  // BUILDFLAG(IS_ANDROID)
  return false;
}

const std::string GetCurrentCountryCode() {
  const country_codes::CountryId country_id =
      country_codes::GetCurrentCountryID();
  const std::string_view country_code =
      country_id.IsValid() ? country_id.CountryCode() : "US";
  return base::ToUpperASCII(country_code);
}

bool IsSupportedCountryCode() {
  return !brave_l10n::IsISOCountryCodeOFACSanctioned(GetCurrentCountryCode());
}

}  // namespace

bool IsSupported(PrefService* prefs, IsSupportedOptions options) {
  bool is_supported = !IsDisabledByPolicy(prefs) && !IsDisabledByFeature();
  if (is_supported && options != IsSupportedOptions::kSkipRegionCheck) {
    return IsSupportedCountryCode();
  }
  return is_supported;
}

}  // namespace brave_rewards
