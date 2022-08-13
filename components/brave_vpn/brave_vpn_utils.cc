/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_utils.h"

#include "base/feature_list.h"
#include "base/notreached.h"
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/features.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave_vpn {

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

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
#if !BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(prefs::kBraveVPNShowButton, true);
  registry->RegisterListPref(prefs::kBraveVPNRegionList);
  registry->RegisterStringPref(prefs::kBraveVPNDeviceRegion, "");
  registry->RegisterStringPref(prefs::kBraveVPNSelectedRegion, "");
  registry->RegisterBooleanPref(prefs::kBraveVPNShowDNSPolicyWarningDialog,
                                true);
#elif BUILDFLAG(IS_ANDROID)
  registry->RegisterStringPref(prefs::kBraveVPNPurchaseTokenAndroid, "");
  registry->RegisterStringPref(prefs::kBraveVPNPackageAndroid, "");
#endif
  registry->RegisterStringPref(prefs::kBraveVPNEEnvironment,
                               skus::GetDefaultEnvironment());
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, prefs::kBraveVPNFirstUseTime, prefs::kBraveVPNLastUseTime,
      prefs::kBraveVPNUsedSecondDay, prefs::kBraveVPNDaysInMonthUsed);
}

}  // namespace brave_vpn
