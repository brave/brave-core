/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_SEGMENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_SEGMENT_INFO_H_

#include <string>
#include <vector>

namespace brave_ads {

struct CatalogSegmentInfo final {
  std::string code;
  std::string name;
};

bool operator==(const CatalogSegmentInfo&, const CatalogSegmentInfo&);
bool operator!=(const CatalogSegmentInfo&, const CatalogSegmentInfo&);

using CatalogSegmentList = std::vector<CatalogSegmentInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_SEGMENT_INFO_H_
