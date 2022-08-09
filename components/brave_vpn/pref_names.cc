/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/pref_names.h"

#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave_vpn {
namespace prefs {

#if BUILDFLAG(IS_ANDROID)
const char kBraveVPNPurchaseTokenAndroid[] =
    "brave.brave_vpn.purchase_token_android";
const char kBraveVPNPackageAndroid[] = "brave.brave_vpn.package_android";
#endif

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
#if !BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(kBraveVPNShowButton, true);
  registry->RegisterListPref(kBraveVPNRegionList);
  registry->RegisterStringPref(kBraveVPNDeviceRegion, "");
  registry->RegisterStringPref(kBraveVPNSelectedRegion, "");
  registry->RegisterBooleanPref(kBraveVPNShowDNSPolicyWarningDialog, true);
#elif BUILDFLAG(IS_ANDROID)
  registry->RegisterStringPref(kBraveVPNPurchaseTokenAndroid, "");
  registry->RegisterStringPref(kBraveVPNPackageAndroid, "");
#endif
  registry->RegisterStringPref(kBraveVPNEEnvironment,
                               skus::GetDefaultEnvironment());
}

}  // namespace prefs

}  // namespace brave_vpn
