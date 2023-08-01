/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_PREF_NAMES_H_

#include "build/build_config.h"

namespace brave_vpn {
namespace prefs {
constexpr char kManagedBraveVPNDisabled[] =
    "brave.brave_vpn.disabled_by_policy";
constexpr char kBraveVPNLocalStateMigrated[] = "brave.brave_vpn.migrated";
constexpr char kBraveVPNRootPref[] = "brave.brave_vpn";
constexpr char kBraveVPNShowButton[] = "brave.brave_vpn.show_button";
constexpr char kBraveVPNRegionList[] = "brave.brave_vpn.region_list";
// Cached fetched date for trying to refresh region_list once per day
constexpr char kBraveVPNRegionListFetchedDate[] =
    "brave.brave_vpn.region_list_fetched_date";
constexpr char kBraveVPNDeviceRegion[] = "brave.brave_vpn.device_region_name";
constexpr char kBraveVPNSelectedRegion[] =
    "brave.brave_vpn.selected_region_name";
#if BUILDFLAG(IS_WIN)
constexpr char kBraveVpnShowDNSPolicyWarningDialog[] =
    "brave.brave_vpn.show_dns_policy_warning_dialog";
constexpr char kBraveVPNShowNotificationDialog[] =
    "brave.brave_vpn.show_notification_dialog";
constexpr char kBraveVPNWireguardFallbackDialog[] =
    "brave.brave_vpn.show_wireguard_fallback_dialog";
#endif  // BUILDFLAG(IS_WIN)
constexpr char kBraveVPNWireguardProfileCredentials[] =
    "brave.brave_vpn.wireguard.profile_credentials";
constexpr char kBraveVPNEnvironment[] = "brave.brave_vpn.env";
// Dict that has subscriber credential its expiration date.
constexpr char kBraveVPNSubscriberCredential[] =
    "brave.brave_vpn.subscriber_credential";

// Time that session expired occurs.
constexpr char kBraveVPNSessionExpiredDate[] =
    "brave.brave_vpn.session_expired_date";

#if BUILDFLAG(IS_ANDROID)
extern const char kBraveVPNPurchaseTokenAndroid[];
extern const char kBraveVPNPackageAndroid[];
extern const char kBraveVPNProductIdAndroid[];
#endif

constexpr char kBraveVPNFirstUseTime[] = "brave.brave_vpn.first_use_time";
constexpr char kBraveVPNLastUseTime[] = "brave.brave_vpn.last_use_time";
constexpr char kBraveVPNUsedSecondDay[] = "brave.brave_vpn.used_second_day";
constexpr char kBraveVPNDaysInMonthUsed[] =
    "brave.brave_vpn.days_in_month_used";
}  // namespace prefs

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_PREF_NAMES_H_
