// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONSTANTS_H_

#include "base/no_destructor.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace brave_vpn {
constexpr webui::LocalizedString kLocalizedStrings[] = {
    {"braveVpn", IDS_BRAVE_VPN},
    {"braveVpnConnect", IDS_BRAVE_VPN_CONNECT},
    {"braveVpnConnecting", IDS_BRAVE_VPN_CONNECTING},
    {"braveVpnConnected", IDS_BRAVE_VPN_CONNECTED},
    {"braveVpnDisconnecting", IDS_BRAVE_VPN_DISCONNECTING},
    {"braveVpnDisconnected", IDS_BRAVE_VPN_DISCONNECTED},
    {"braveVpnConnectionFailed", IDS_BRAVE_VPN_CONNECTION_FAILED},
    {"braveVpnUnableConnectToServer", IDS_BRAVE_VPN_UNABLE_CONNECT_TO_SERVER},
    {"braveVpnTryAgain", IDS_BRAVE_VPN_TRY_AGAIN},
    {"braveVpnChooseAnotherServer", IDS_BRAVE_VPN_CHOOSE_ANOTHER_SERVER},
    {"braveVpnUnableConnectInfo", IDS_BRAVE_VPN_UNABLE_CONNECT_INFO},
    {"braveVpnBuy", IDS_BRAVE_VPN_BUY},
    {"braveVpnPurchased", IDS_BRAVE_VPN_HAS_PURCHASED},
    {"braveVpnPoweredBy", IDS_BRAVE_VPN_POWERED_BY},
    {"braveVpnSettingsPanelHeader", IDS_BRAVE_VPN_SETTINGS_PANEL_HEADER},
    {"braveVpnStatus", IDS_BRAVE_VPN_STATUS},
    {"braveVpnExpires", IDS_BRAVE_VPN_EXPIRES},
    {"braveVpnManageSubscription", IDS_BRAVE_VPN_MANAGE_SUBSCRIPTION},
    {"braveVpnContactSupport", IDS_BRAVE_VPN_CONTACT_SUPPORT},
    {"braveVpnAbout", IDS_BRAVE_VPN_ABOUT},
    {"braveVpnFeature1", IDS_BRAVE_VPN_FEATURE_1},
    {"braveVpnFeature2", IDS_BRAVE_VPN_FEATURE_2},
    {"braveVpnFeature3", IDS_BRAVE_VPN_FEATURE_3},
    {"braveVpnFeature4", IDS_BRAVE_VPN_FEATURE_4},
    {"braveVpnFeature5", IDS_BRAVE_VPN_FEATURE_5},
    {"braveVpnLoading", IDS_BRAVE_VPN_LOADING},
    {"braveVpnPurchaseFailed", IDS_BRAVE_VPN_PURCHASE_FAILED},
    {"braveVpnSupportTicketFailed", IDS_BRAVE_VPN_SUPPORT_TICKET_FAILED},
    {"braveVpnEditPaymentMethod", IDS_BRAVE_VPN_EDIT_PAYMENT},
    {"braveVpnPaymentFailure", IDS_BRAVE_VPN_PAYMENT_FAILURE},
    {"braveVpnPaymentFailureReason", IDS_BRAVE_VPN_PAYMENT_FAILURE_REASON},
    {"braveVpnSupportEmail", IDS_BRAVE_VPN_SUPPORT_EMAIL},
    {"braveVpnSupportSubject", IDS_BRAVE_VPN_SUPPORT_SUBJECT},
    {"braveVpnSupportSubjectNotSet", IDS_BRAVE_VPN_SUPPORT_SUBJECT_NOTSET},
    {"braveVpnSupportSubjectOtherConnectionProblem",
     IDS_BRAVE_VPN_SUPPORT_SUBJECT_OTHER_CONNECTION_PROBLEM},
    {"braveVpnSupportSubjectNoInternet",
     IDS_BRAVE_VPN_SUPPORT_SUBJECT_NO_INTERNET},
    {"braveVpnSupportSubjectSlowConnection",
     IDS_BRAVE_VPN_SUPPORT_SUBJECT_SLOW_CONNECTION},
    {"braveVpnSupportSubjectWebsiteDoesntWork",
     IDS_BRAVE_VPN_SUPPORT_SUBJECT_WEBSITE_DOESNT_WORK},
    {"braveVpnSupportSubjectOther", IDS_BRAVE_VPN_SUPPORT_SUBJECT_OTHER},
    {"braveVpnSupportBody", IDS_BRAVE_VPN_SUPPORT_BODY},
    {"braveVpnSupportOptionalHeader", IDS_BRAVE_VPN_SUPPORT_OPTIONAL_HEADER},
    {"braveVpnSupportOptionalNotes", IDS_BRAVE_VPN_SUPPORT_OPTIONAL_NOTES},
    {"braveVpnSupportOptionalNotesPrivacyPolicy",
     IDS_BRAVE_VPN_SUPPORT_OPTIONAL_NOTES_PRIVACY_POLICY},
    {"braveVpnSupportOptionalVpnHostname",
     IDS_BRAVE_VPN_SUPPORT_OPTIONAL_VPN_HOSTNAME},
    {"braveVpnSupportOptionalAppVersion",
     IDS_BRAVE_VPN_SUPPORT_OPTIONAL_APP_VERSION},
    {"braveVpnSupportOptionalOsVersion",
     IDS_BRAVE_VPN_SUPPORT_OPTIONAL_OS_VERSION},
    {"braveVpnSupportNotes", IDS_BRAVE_VPN_SUPPORT_NOTES},
    {"braveVpnSupportSubmit", IDS_BRAVE_VPN_SUPPORT_SUBMIT},
    {"braveVpnConnectNotAllowed", IDS_BRAVE_VPN_CONNECT_NOT_ALLOWED},
};

constexpr char kManageUrlProd[] = "https://account.brave.com/account/";
constexpr char kManageUrlStaging[] =
    "https://account.bravesoftware.com/account/";
constexpr char kManageUrlDev[] = "https://account.brave.software/account/";

// TODO(simonhong): Update when vpn feedback url is ready.
constexpr char kFeedbackUrl[] = "https://support.brave.com/";
constexpr char kAboutUrl[] = "https://brave.com/firewall-vpn/";

constexpr char kBraveVPNEntryName[] = "BraveVPN";
constexpr char kRegionContinentKey[] = "continent";
constexpr char kRegionNameKey[] = "name";
constexpr char kRegionNamePrettyKey[] = "name-pretty";
constexpr char kRegionCountryIsoCodeKey[] = "country-iso-code";
constexpr char kCreateSupportTicket[] = "api/v1.2/partners/support-ticket";

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONSTANTS_H_
