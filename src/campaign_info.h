/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>

#include "catalog_geo_target_info.h"
#include "catalog_creative_set_info.h"

namespace ads {

struct CampaignInfo {
  CampaignInfo() :
      campaign_id(""),
      name(""),
      start_at(""),
      end_at(""),
      daily_cap(0),
      budget(0),
      advertiser_id(""),
      geo_targets({}),
      creative_sets({}) {}

  std::string campaign_id;
  std::string name;
  std::string start_at;
  std::string end_at;
  uint64_t daily_cap;
  uint64_t budget;
  std::string advertiser_id;
  std::vector<GeoTargetInfo> geo_targets;
  std::vector<CreativeSetInfo> creative_sets;
};

}  // namespace ads
