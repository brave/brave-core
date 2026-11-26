/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_helper.h"

#include "base/check.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

namespace brave_vpn {

mojom::RegionPtr GetRegionPtrWithNameFromRegionList(
    const std::string& name,
    const std::vector<mojom::RegionPtr>& region_list) {
  for (const auto& region : region_list) {
    // Return if it's country precision.
    if (region->name == name) {
      return region.Clone();
    } else {
      for (const auto& city : region->cities) {
        // Try to find from cities if |name| is country precision in |region|.
        if (city->name == name) {
          return city.Clone();
        }
      }
    }
  }

  // We should not reach here but if service(guardian) api give wrong list,
  // it could be crashed if we return null.
  // Instead, return first region in the list for safe.
  CHECK(!region_list.empty());
  return region_list[0].Clone();
}

}  // namespace brave_vpn
