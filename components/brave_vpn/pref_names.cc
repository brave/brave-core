/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/pref_names.h"

#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave_vpn {
namespace prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kBraveVPNShowButton, true);
  registry->RegisterListPref(kBraveVPNRegionList);
  registry->RegisterStringPref(kBraveVPNDeviceRegion, "");
  registry->RegisterStringPref(kBraveVPNSelectedRegion, "");
  registry->RegisterBooleanPref(kBraveVPNShowDNSPolicyWarningDialog, true);
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, kBraveVPNFirstUseTime, kBraveVPNLastUseTime,
      kBraveVPNUsedSecondDay, kBraveVPNDaysInMonthUsed);
}

}  // namespace prefs

}  // namespace brave_vpn
