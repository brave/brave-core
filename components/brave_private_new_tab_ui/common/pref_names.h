/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PRIVATE_NEW_TAB_UI_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_PRIVATE_NEW_TAB_UI_COMMON_PREF_NAMES_H_

class PrefRegistrySimple;

namespace brave_private_new_tab::prefs {

inline constexpr char kBravePrivateWindowDisclaimerDismissed[] =
    "brave.brave_private_new_tab.private_window_disclaimer_dismissed";
inline constexpr char kBraveTorWindowDisclaimerDismissed[] =
    "brave.brave_private_new_tab.tor_window_disclaimer_dismissed";
// Last known answer to whether the Brave VPN promo card should be shown, so
// the page can render it right away while the VPN purchase state loads.
// Holds a VpnPromoCache value.
inline constexpr char kBravePrivateWindowVpnPromoCache[] =
    "brave.brave_private_new_tab.vpn_promo_cache";

enum class VpnPromoCache : int {
  kUnknown = 0,
  kShow = 1,
  kHide = 2,
};

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace brave_private_new_tab::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_PRIVATE_NEW_TAB_UI_COMMON_PREF_NAMES_H_
