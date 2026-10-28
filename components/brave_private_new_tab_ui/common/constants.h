/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PRIVATE_NEW_TAB_UI_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_PRIVATE_NEW_TAB_UI_COMMON_CONSTANTS_H_

#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace brave_private_new_tab {

inline constexpr webui::LocalizedString kLocalizedStrings[] = {
    {"headingPrivate", IDS_BRAVE_PRIVATE_NEW_TAB_HEADING_PRIVATE},
    {"headingTor", IDS_BRAVE_PRIVATE_NEW_TAB_HEADING_TOR},
    {"hideInfoLabel", IDS_BRAVE_PRIVATE_NEW_TAB_HIDE_INFO},
    {"showInfoLabel", IDS_BRAVE_PRIVATE_NEW_TAB_SHOW_INFO},
    {"cardNoTraceTitle", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_NO_TRACE_TITLE},
    {"cardNoTraceDesc", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_NO_TRACE_DESC},
    {"cardVisibilityTitle", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_VISIBILITY_TITLE},
    {"cardVisibilityDesc", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_VISIBILITY_DESC},
    {"cardVpnTitle", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_VPN_TITLE},
    {"cardVpnDesc", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_VPN_DESC},
    {"cardTorPromoTitle", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_TOR_PROMO_TITLE},
    {"cardTorPromoDesc", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_TOR_PROMO_DESC},
    {"cardAboutTorTitle", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_ABOUT_TOR_TITLE},
    {"cardAboutTorDesc", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_ABOUT_TOR_DESC},
    {"cardTorSafetyTitle", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_TOR_SAFETY_TITLE},
    {"cardTorSafetyDesc", IDS_BRAVE_PRIVATE_NEW_TAB_CARD_TOR_SAFETY_DESC},
    {"torStatusConnected",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_CONNECTED},  // NOLINT
    {"torStatusDisconnected",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_DISCONNECTED},  // NOLINT
    {"torStatusInitializing",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_INITIALIZING},  // NOLINT
    {"torStatusConnectionSlow",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_CONNECTION_SLOW},
    {"torStatusConnectionSlowDesc",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_CONNECTION_SLOW_DESC},
    {"torStatusConnectionFailed",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_CONNECTION_FAILED},
    {"torHelpDisconnectedReenable",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_HELP_DISCONNECTED_REENABLE},  // NOLINT
    {"torHelpDisconnectedBridges",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_HELP_DISCONNECTED_BRIDGES},  // NOLINT
    {"searchPlaceholderLabel",
     IDS_BRAVE_PRIVATE_NEW_TAB_SEARCH_PLACEHOLDER},  // NOLINT
    {"searchSubmitLabel", IDS_BRAVE_PRIVATE_NEW_TAB_SEARCH_SUBMIT},
};

}  // namespace brave_private_new_tab

#endif  // BRAVE_COMPONENTS_BRAVE_PRIVATE_NEW_TAB_UI_COMMON_CONSTANTS_H_
