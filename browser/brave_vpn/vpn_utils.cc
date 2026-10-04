/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/vpn_utils.h"

#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_vpn {

bool IsAllowedForContext(content::BrowserContext* context) {
  return Profile::FromBrowserContext(context)->IsRegularProfile() &&
         brave_vpn::IsBraveVPNFeatureEnabled();
}

bool IsBraveVPNEnabled(content::BrowserContext* context) {
  // TODO(simonhong): Can we use this check for android?
  // For now, vpn is disabled by default on desktop but not sure on
  // android.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  return brave_vpn::IsBraveVPNEnabled(user_prefs::UserPrefs::Get(context)) &&
         IsAllowedForContext(context);
#else
  return Profile::FromBrowserContext(context)->IsRegularProfile();
#endif
}

}  // namespace brave_vpn
