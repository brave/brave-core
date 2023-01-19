/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CATALOG_GEO_TARGET_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CATALOG_GEO_TARGET_INFO_H_

#include <string>
#include <vector>

namespace ads {

struct CatalogGeoTargetInfo final {
  std::string code;
  std::string name;
};

bool operator==(const CatalogGeoTargetInfo& lhs,
                const CatalogGeoTargetInfo& rhs);
bool operator!=(const CatalogGeoTargetInfo& lhs,
                const CatalogGeoTargetInfo& rhs);

using CatalogGeoTargetList = std::vector<CatalogGeoTargetInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CATALOG_GEO_TARGET_INFO_H_
