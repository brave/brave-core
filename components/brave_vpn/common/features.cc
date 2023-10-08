/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace brave_vpn {

namespace features {

BASE_FEATURE(kBraveVPN,
             "BraveVPN",
#if !BUILDFLAG(IS_LINUX)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

BASE_FEATURE(kBraveVPNLinkSubscriptionAndroidUI,
             "BraveVPNLinkSubscriptionAndroidUI",
             base::FEATURE_ENABLED_BY_DEFAULT);

#if BUILDFLAG(IS_WIN)
BASE_FEATURE(kBraveVPNDnsProtection,
             "BraveVPNDnsProtection",
             base::FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE(kBraveVPNUseWireguardService,
             "BraveVPNUseWireguardService",
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif
#if BUILDFLAG(IS_MAC)
BASE_FEATURE(kBraveVPNEnableWireguardForOSX,
             "kBraveVPNEnableWireguardForOSX",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif
}  // namespace features

}  // namespace brave_vpn
