/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_PREF_NAMES_H_

#include "build/build_config.h"

class PrefRegistrySimple;

namespace brave_vpn {
namespace prefs {

constexpr char kBraveVPNShowButton[] = "brave.brave_vpn.show_button";
constexpr char kBraveVPNRegionList[] = "brave.brave_vpn.region_list";
constexpr char kBraveVPNDeviceRegion[] = "brave.brave_vpn.device_region_name";
constexpr char kBraveVPNSelectedRegion[] =
    "brave.brave_vpn.selected_region_name";
constexpr char kBraveVPNShowDNSPolicyWarningDialog[] =
    "brave.brave_vpn.show_dns_policy_warning_dialog";
constexpr char kBraveVPNEEnvironment[] = "brave.brave_vpn.env";

#if BUILDFLAG(IS_ANDROID)
extern const char kBraveVPNPurchaseTokenAndroid[];
extern const char kBraveVPNPackageAndroid[];
#endif

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace prefs

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_PREF_NAMES_H_
