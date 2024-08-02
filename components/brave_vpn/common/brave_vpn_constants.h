/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_BRAVE_VPN_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_BRAVE_VPN_CONSTANTS_H_

#include "build/buildflag.h"

namespace brave_vpn {

inline constexpr char kManageUrlProd[] = "https://account.brave.com/account/";
inline constexpr char kManageUrlStaging[] =
    "https://account.bravesoftware.com/account/";
inline constexpr char kManageUrlDev[] =
    "https://account.brave.software/account/";

// TODO(simonhong): Update when vpn feedback url is ready.
inline constexpr char kFeedbackUrl[] = "https://support.brave.com/";
inline constexpr char kAboutUrl[] = "https://brave.com/firewall-vpn/";

inline constexpr char kRegionNameKey[] = "name";
inline constexpr char kRegionNamePrettyKey[] = "name-pretty";
inline constexpr char kRegionContinentKey[] = "continent";
inline constexpr char kRegionCountryIsoCodeKey[] = "country-iso-code";
inline constexpr char kRegionPrecisionKey[] = "region-precision";
inline constexpr char kRegionCitiesKey[] = "cities";
inline constexpr char kRegionLatitudeKey[] = "latitude";
inline constexpr char kRegionLongitudeKey[] = "longitude";
inline constexpr char kRegionServerCountKey[] = "server-count";

inline constexpr char kCreateSupportTicket[] =
    "api/v1.2/partners/support-ticket";
inline constexpr char kSupportTicketEmailKey[] = "email";
inline constexpr char kSupportTicketSubjectKey[] = "subject";
inline constexpr char kSupportTicketSupportTicketKey[] = "support-ticket";
inline constexpr char kSupportTicketPartnerClientIdKey[] = "partner-client-id";
inline constexpr char kSupportTicketTimezoneKey[] = "timezone";

inline constexpr char kVpnHost[] = "connect-api.guardianapp.com";
inline constexpr char kAllServerRegions[] = "api/v1/servers/all-server-regions";
inline constexpr char kServerRegionsWithCities[] =
    "api/v1.3/servers/all-server-regions/city-by-country";
inline constexpr char kTimezonesForRegions[] =
    "api/v1.1/servers/timezones-for-regions";
inline constexpr char kHostnameForRegion[] =
    "api/v1.2/servers/hostnames-for-region";
inline constexpr char kProfileCredential[] = "api/v1.1/register-and-create";
inline constexpr char kCredential[] = "api/v1.3/device/";
inline constexpr char kVerifyPurchaseToken[] = "api/v1.1/verify-purchase-token";
inline constexpr char kCreateSubscriberCredentialV12[] =
    "api/v1.2/subscriber-credential/create";
constexpr int kP3AIntervalHours = 24;

inline constexpr char kSubscriberCredentialKey[] = "credential";
inline constexpr char kSkusCredentialKey[] = "skus_credential";
inline constexpr char kRetriedSkusCredentialKey[] = "retried_skus_credential";
inline constexpr char kSubscriberCredentialExpirationKey[] = "expiration";

#if !BUILDFLAG(IS_ANDROID)
inline constexpr char kTokenNoLongerValid[] = "Token No Longer Valid";
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_BRAVE_VPN_CONSTANTS_H_
