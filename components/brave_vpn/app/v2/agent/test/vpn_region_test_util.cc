/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/test/vpn_region_test_util.h"

#include <utility>

#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"

namespace brave_vpn::v2 {

mojom::VpnRegionPtr MakeTestRegion(std::string name) {
  auto region = mojom::VpnRegion::New();
  region->name = std::move(name);
  region->region_precision = mojom::RegionPrecision::kCountry;
  return region;
}

mojom::VpnRegionSelectionPtr MakeTestPickedSelection(std::string name) {
  return mojom::VpnRegionSelection::New(/*is_automatic=*/false,
                                        MakeTestRegion(std::move(name)));
}

mojom::VpnRegionSelectionPtr MakeTestAutomaticSelection(
    std::optional<std::string> name) {
  return mojom::VpnRegionSelection::New(
      /*is_automatic=*/true, name ? MakeTestRegion(std::move(*name)) : nullptr);
}

}  // namespace brave_vpn::v2
