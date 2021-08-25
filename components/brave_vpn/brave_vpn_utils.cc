/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_utils.h"

#include "base/feature_list.h"
#include "brave/components/brave_vpn/features.h"

namespace brave_vpn {

bool IsBraveVPNEnabled() {
  return base::FeatureList::IsEnabled(brave_vpn::features::kBraveVPN);
}

}  // namespace brave_vpn
