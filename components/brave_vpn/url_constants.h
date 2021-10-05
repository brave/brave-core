/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_URL_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_URL_CONSTANTS_H_

namespace brave_vpn {

constexpr char kManageUrlProd[] = "http://account.brave.com/";
constexpr char kManageUrlStaging[] = "http://account.bravesoftware.com/";
constexpr char kManageUrlDev[] = "https://account.brave.software/";

// TODO(simonhong): Update when vpn feedback url is ready.
constexpr char kFeedbackUrl[] = "http://support.brave.com/";

constexpr char kAboutUrl[] = "https://brave.com/firewall-vpn/";

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_URL_CONSTANTS_H_
