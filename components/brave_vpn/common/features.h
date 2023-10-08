/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "build/build_config.h"

namespace brave_vpn {
namespace features {

BASE_DECLARE_FEATURE(kBraveVPN);
BASE_DECLARE_FEATURE(kBraveVPNLinkSubscriptionAndroidUI);
#if BUILDFLAG(IS_WIN)
BASE_DECLARE_FEATURE(kBraveVPNDnsProtection);
BASE_DECLARE_FEATURE(kBraveVPNUseWireguardService);
#endif
#if BUILDFLAG(IS_MAC)
BASE_DECLARE_FEATURE(kBraveVPNEnableWireguardForOSX);
#endif
}  // namespace features
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_FEATURES_H_
