/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PRIVATE_NEW_TAB_UI_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_PRIVATE_NEW_TAB_UI_COMMON_CONSTANTS_H_

#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace brave_private_new_tab {

constexpr webui::LocalizedString kLocalizedStrings[] = {
    {"headerTitle", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW},
    {"headerText", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_DESC},
    {"headerText1", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_DESC1},
    {"headerText2", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_DESC2},
    {"headerButton", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_BUTTON},
    {"headerTorTitle", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_TOR},
    {"headerTorText", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_TOR_DESC},
    {"headerTorText1", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_TOR_TEXT_1},
    {"headerTorText2", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_TOR_TEXT_2},
    {"torStatus", IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS},
    {"torStatusConnected",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_CONNECTED},  // NOLINT
    {"torStatusDisconnected",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_DISCONNECTED},  // NOLINT
    {"torStatusInitializing",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_INITIALIZING},  // NOLINT
    {"torHelpConnecting", IDS_BRAVE_PRIVATE_NEW_TAB_TOR_HELP_CONNECTING},
    {"torHelpDisconnected",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_HELP_DISCONNECTED},  // NOLINT
    {"torHelpContactSupport",
     IDS_BRAVE_PRIVATE_NEW_TAB_TOR_HELP_CONTACT_SUPPORT},  // NOLINT
    {"searchPlaceholderLabel",
     IDS_BRAVE_PRIVATE_NEW_TAB_SEARCH_PLACEHOLDER},  // NOLINT
};

}  // namespace brave_private_new_tab

#endif  // BRAVE_COMPONENTS_BRAVE_PRIVATE_NEW_TAB_UI_COMMON_CONSTANTS_H_
