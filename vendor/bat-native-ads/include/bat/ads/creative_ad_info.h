/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CREATIVE_AD_INFO_H_
#define BAT_ADS_CREATIVE_AD_INFO_H_

#include <string>
#include <vector>

namespace ads {

struct CreativeAdInfo {
  CreativeAdInfo();
  CreativeAdInfo(
      const CreativeAdInfo& info);
  ~CreativeAdInfo();

  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string start_at_timestamp;
  std::string end_at_timestamp;
  unsigned int daily_cap = 0;
  unsigned int per_day = 0;
  unsigned int total_max = 0;
  std::string category;
  std::vector<std::string> geo_targets;
};

}  // namespace ads

#endif  // BAT_ADS_CREATIVE_AD_INFO_H_
