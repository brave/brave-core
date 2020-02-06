/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_INFO_H_
#define BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/catalog_geo_target_info.h"
#include "bat/ads/internal/catalog_day_part_info.h"
#include "bat/ads/internal/catalog_creative_set_info.h"

namespace ads {

struct CatalogCampaignInfo {
  CatalogCampaignInfo();
  CatalogCampaignInfo(
      const CatalogCampaignInfo& info);
  ~CatalogCampaignInfo();

  std::string campaign_id;
  unsigned int priority = 0;
  std::string name;
  std::string start_at;
  std::string end_at;
  unsigned int daily_cap = 0;
  std::string advertiser_id;
  std::vector<CatalogGeoTargetInfo> geo_targets;
  std::vector<CatalogDayPartInfo> day_parts;
  std::vector<CatalogCreativeSetInfo> creative_sets;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_INFO_H_
