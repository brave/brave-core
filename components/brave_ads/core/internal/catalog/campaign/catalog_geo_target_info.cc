/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/catalog_geo_target_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const CatalogGeoTargetInfo& lhs,
                const CatalogGeoTargetInfo& rhs) {
  const auto tie = [](const CatalogGeoTargetInfo& geo_target) {
    return std::tie(geo_target.code, geo_target.name);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CatalogGeoTargetInfo& lhs,
                const CatalogGeoTargetInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
