/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/vpn_utils.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "build/build_config.h"

namespace brave_vpn {

bool IsBraveVPNEnabled(content::BrowserContext* context) {
  // TODO(simonhong): Can we use this check for android?
  // For now, vpn is disabled by default on desktop but not sure on
  // android.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  return brave_vpn::IsBraveVPNEnabled() && brave::IsRegularProfile(context);
#else
  return brave::IsRegularProfile(context);
#endif
}

}  // namespace brave_vpn
