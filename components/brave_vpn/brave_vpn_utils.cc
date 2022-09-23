/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_utils.h"

#include "base/feature_list.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/features.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "build/build_config.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {

namespace {

void RegisterVPNLocalStatePrefs(PrefRegistrySimple* registry) {
#if !BUILDFLAG(IS_ANDROID)
  registry->RegisterListPref(prefs::kBraveVPNRegionList);
  registry->RegisterStringPref(prefs::kBraveVPNDeviceRegion, "");
  registry->RegisterStringPref(prefs::kBraveVPNSelectedRegion, "");
  registry->RegisterBooleanPref(prefs::kBraveVPNShowDNSPolicyWarningDialog,
                                true);
#endif
  registry->RegisterStringPref(prefs::kBraveVPNEEnvironment,
                               skus::GetDefaultEnvironment());
  registry->RegisterDictionaryPref(prefs::kBraveVPNRootPref);

  registry->RegisterBooleanPref(prefs::kBraveVPNLocalStateMigrated, false);
}

}  // namespace

void MigrateVPNSettings(PrefService* profile_prefs, PrefService* local_prefs) {
  if (local_prefs->GetBoolean(prefs::kBraveVPNLocalStateMigrated)) {
    return;
  }

  if (!profile_prefs->HasPrefPath(prefs::kBraveVPNRootPref)) {
    local_prefs->SetBoolean(prefs::kBraveVPNLocalStateMigrated, true);
    return;
  }
  base::Value::Dict obsolete_pref =
      profile_prefs->GetDict(prefs::kBraveVPNRootPref).Clone();
  base::Value::Dict result;
  if (local_prefs->HasPrefPath(prefs::kBraveVPNRootPref)) {
    result = local_prefs->GetDict(prefs::kBraveVPNRootPref).Clone();
    auto& result_dict = result;
    result_dict.Merge(std::move(obsolete_pref));
  } else {
    result = std::move(obsolete_pref);
  }
  // Do not migrate brave_vpn::prefs::kBraveVPNShowButton, we want it to be
  // inside the profile preferences.
  auto tokens =
      base::SplitString(brave_vpn::prefs::kBraveVPNShowButton, ".",
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (result.FindBool(tokens.back())) {
    result.Remove(tokens.back());
  }
  local_prefs->Set(prefs::kBraveVPNRootPref, base::Value(std::move(result)));
  local_prefs->SetBoolean(prefs::kBraveVPNLocalStateMigrated, true);

  bool show_button =
      profile_prefs->GetBoolean(brave_vpn::prefs::kBraveVPNShowButton);
  profile_prefs->ClearPref(prefs::kBraveVPNRootPref);
  // Set kBraveVPNShowButton back, it is only one per profile preference for
  // now.
  profile_prefs->SetBoolean(brave_vpn::prefs::kBraveVPNShowButton, show_button);
}

bool IsBraveVPNEnabled() {
  return base::FeatureList::IsEnabled(brave_vpn::features::kBraveVPN) &&
         base::FeatureList::IsEnabled(skus::features::kSkusFeature);
}

std::string GetManageUrl(const std::string& env) {
  if (env == skus::kEnvProduction)
    return brave_vpn::kManageUrlProd;
  if (env == skus::kEnvStaging)
    return brave_vpn::kManageUrlStaging;
  if (env == skus::kEnvDevelopment)
    return brave_vpn::kManageUrlDev;

  NOTREACHED();
  return brave_vpn::kManageUrlProd;
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kBraveVPNRootPref);
  registry->RegisterBooleanPref(prefs::kBraveVPNShowButton, true);
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterStringPref(prefs::kBraveVPNPurchaseTokenAndroid, "");
  registry->RegisterStringPref(prefs::kBraveVPNPackageAndroid, "");
  registry->RegisterStringPref(prefs::kBraveVPNProductIdAndroid, "");
#endif
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, prefs::kBraveVPNFirstUseTime, prefs::kBraveVPNLastUseTime,
      prefs::kBraveVPNUsedSecondDay, prefs::kBraveVPNDaysInMonthUsed);
  RegisterVPNLocalStatePrefs(registry);
}

}  // namespace brave_vpn
