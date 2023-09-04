/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_BRAVE_VPN_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_BRAVE_VPN_CONSTANTS_H_

#include "build/buildflag.h"

namespace brave_vpn {

constexpr char kManageUrlProd[] = "https://account.brave.com/account/";
constexpr char kManageUrlStaging[] =
    "https://account.bravesoftware.com/account/";
constexpr char kManageUrlDev[] = "https://account.brave.software/account/";

// TODO(simonhong): Update when vpn feedback url is ready.
constexpr char kFeedbackUrl[] = "https://support.brave.com/";
constexpr char kAboutUrl[] = "https://brave.com/firewall-vpn/";

constexpr char kRegionContinentKey[] = "continent";
constexpr char kRegionNameKey[] = "name";
constexpr char kRegionNamePrettyKey[] = "name-pretty";
constexpr char kRegionCountryIsoCodeKey[] = "country-iso-code";
constexpr char kCreateSupportTicket[] = "api/v1.2/partners/support-ticket";
constexpr char kSupportTicketEmailKey[] = "email";
constexpr char kSupportTicketSubjectKey[] = "subject";
constexpr char kSupportTicketSupportTicketKey[] = "support-ticket";
constexpr char kSupportTicketPartnerClientIdKey[] = "partner-client-id";
constexpr char kSupportTicketTimezoneKey[] = "timezone";

constexpr char kVpnHost[] = "connect-api.guardianapp.com";
constexpr char kAllServerRegions[] = "api/v1/servers/all-server-regions";
constexpr char kTimezonesForRegions[] =
    "api/v1.1/servers/timezones-for-regions";
constexpr char kHostnameForRegion[] = "api/v1.2/servers/hostnames-for-region";
constexpr char kProfileCredential[] = "api/v1.1/register-and-create";
constexpr char kCredential[] = "api/v1.3/device/";
constexpr char kVerifyPurchaseToken[] = "api/v1.1/verify-purchase-token";
constexpr char kCreateSubscriberCredentialV12[] =
    "api/v1.2/subscriber-credential/create";
constexpr int kP3AIntervalHours = 24;

constexpr char kSubscriberCredentialKey[] = "credential";
constexpr char kSkusCredentialKey[] = "skus_credential";
constexpr char kRetriedSkusCredentialKey[] = "retried_skus_credential";
constexpr char kSubscriberCredentialExpirationKey[] = "expiration";

#if !BUILDFLAG(IS_ANDROID)
constexpr char kTokenNoLongerValid[] = "Token No Longer Valid";
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_BRAVE_VPN_CONSTANTS_H_
