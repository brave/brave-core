/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_utils.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "brave/components/brave_vpn/features.h"
#include "brave/components/brave_vpn/switches.h"
#include "brave/components/brave_vpn/url_constants.h"

namespace brave_vpn {

bool IsBraveVPNEnabled() {
  return base::FeatureList::IsEnabled(brave_vpn::features::kBraveVPN);
}

std::string GetManageUrl() {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(brave_vpn::switches::kBraveVPNAccountHost)) {
#if defined(OFFICIAL_BUILD)
    return brave_vpn::kManageUrlProd;
#else
    return brave_vpn::kManageUrlDev;
#endif
  }

  const std::string value =
      cmd->GetSwitchValueASCII(brave_vpn::switches::kBraveVPNAccountHost);
  if (value == "prod")
    return brave_vpn::kManageUrlProd;
  if (value == "staging")
    return brave_vpn::kManageUrlStaging;
  if (value == "dev")
    return brave_vpn::kManageUrlDev;

  NOTREACHED();
  return brave_vpn::kManageUrlProd;
}

}  // namespace brave_vpn
