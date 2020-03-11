/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_GEO_TARGET_INFO_H_
#define BAT_ADS_INTERNAL_CATALOG_GEO_TARGET_INFO_H_

#include <string>
#include <vector>

namespace ads {

struct CatalogGeoTargetInfo {
  std::string code;
  std::string name;
};

using CatalogGeoTargetList = std::vector<CatalogGeoTargetInfo>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_GEO_TARGET_INFO_H_
