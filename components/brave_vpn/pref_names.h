/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_PREF_NAMES_H_

class PrefRegistrySimple;

namespace brave_vpn {
namespace prefs {

constexpr char kBraveVPNSelectedRegion[] = "brave.brave_vpn.selected_region";
constexpr char kBraveVPNShowButton[] = "brave.brave_vpn.show_button";

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace prefs

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_PREF_NAMES_H_
