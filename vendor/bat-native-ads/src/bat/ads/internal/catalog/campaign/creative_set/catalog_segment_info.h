/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_SEGMENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_SEGMENT_INFO_H_

#include <string>
#include <vector>

namespace ads {

struct CatalogSegmentInfo final {
  std::string code;
  std::string name;
};

bool operator==(const CatalogSegmentInfo& lhs, const CatalogSegmentInfo& rhs);
bool operator!=(const CatalogSegmentInfo& lhs, const CatalogSegmentInfo& rhs);

using CatalogSegmentList = std::vector<CatalogSegmentInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_SEGMENT_INFO_H_
