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
};
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONSTANTS_H_
