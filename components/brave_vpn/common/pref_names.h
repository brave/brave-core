/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_PREF_NAMES_H_

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "build/build_config.h"

namespace brave_vpn {
namespace prefs {
inline constexpr char kManagedBraveVPNDisabled[] =
    "brave.brave_vpn.disabled_by_policy";
inline constexpr char kBraveVPNLocalStateMigrated[] =
    "brave.brave_vpn.migrated";
inline constexpr char kBraveVPNRootPref[] = "brave.brave_vpn";
inline constexpr char kBraveVPNShowButton[] = "brave.brave_vpn.show_button";
inline constexpr char kBraveVPNRegionList[] = "brave.brave_vpn.region_list";
// Cached fetched date for trying to refresh region_list once per day
inline constexpr char kBraveVPNRegionListFetchedDate[] =
    "brave.brave_vpn.region_list_fetched_date";
inline constexpr char kBraveVPNDeviceRegion[] =
    "brave.brave_vpn.device_region_name";
inline constexpr char kBraveVPNSelectedRegion[] =
    "brave.brave_vpn.selected_region_name";
inline constexpr char kBraveVPNRegionListVersion[] =
    "brave.brave_vpn.region_list_version";
#if BUILDFLAG(IS_WIN)
inline constexpr char kBraveVpnShowDNSPolicyWarningDialog[] =
    "brave.brave_vpn.show_dns_policy_warning_dialog";
inline constexpr char kBraveVPNShowNotificationDialog[] =
    "brave.brave_vpn.show_notification_dialog";
inline constexpr char kBraveVPNWireguardFallbackDialog[] =
    "brave.brave_vpn.show_wireguard_fallback_dialog";
#endif  // BUILDFLAG(IS_WIN)
#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
inline constexpr char kBraveVPNWireguardEnabled[] =
    "brave.brave_vpn.wireguard_enabled";
#endif

#if BUILDFLAG(IS_MAC)
inline constexpr char kBraveVPNOnDemandEnabled[] =
    "brave.brave_vpn.on_demand_enabled";
#endif

inline constexpr char kBraveVPNWireguardProfileCredentials[] =
    "brave.brave_vpn.wireguard.profile_credentials";
inline constexpr char kBraveVPNEnvironment[] = "brave.brave_vpn.env";
// Dict that has subscriber credential its expiration date.
inline constexpr char kBraveVPNSubscriberCredential[] =
    "brave.brave_vpn.subscriber_credential";

// Time that session expired occurs.
inline constexpr char kBraveVPNSessionExpiredDate[] =
    "brave.brave_vpn.session_expired_date";

#if BUILDFLAG(IS_ANDROID)
inline constexpr char kBraveVPNPurchaseTokenAndroid[] =
    "brave.brave_vpn.purchase_token_android";
inline constexpr char kBraveVPNPackageAndroid[] =
    "brave.brave_vpn.package_android";
inline constexpr char kBraveVPNProductIdAndroid[] =
    "brave.brave_vpn.product_id_android";
#endif

inline constexpr char kBraveVPNFirstUseTime[] =
    "brave.brave_vpn.first_use_time";
inline constexpr char kBraveVPNLastUseTime[] = "brave.brave_vpn.last_use_time";
inline constexpr char kBraveVPNUsedSecondDay[] =
    "brave.brave_vpn.used_second_day";
inline constexpr char kBraveVPNDaysInMonthUsed[] =
    "brave.brave_vpn.days_in_month_used";
}  // namespace prefs

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_PREF_NAMES_H_
