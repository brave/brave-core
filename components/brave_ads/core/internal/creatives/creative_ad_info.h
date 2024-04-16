/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_AD_INFO_H_

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "url/gurl.h"

namespace brave_ads {

struct CreativeAdInfo {
  CreativeAdInfo();

  CreativeAdInfo(const CreativeAdInfo&);
  CreativeAdInfo& operator=(const CreativeAdInfo&);

  CreativeAdInfo(CreativeAdInfo&&) noexcept;
  CreativeAdInfo& operator=(CreativeAdInfo&&) noexcept;

  ~CreativeAdInfo();

  bool operator==(const CreativeAdInfo&) const;
  bool operator!=(const CreativeAdInfo&) const;

  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  base::Time start_at;
  base::Time end_at;
  int daily_cap = 0;
  int priority = 0;
  double pass_through_rate = 0.0;
  int per_day = 0;
  int per_week = 0;
  int per_month = 0;
  int total_max = 0;
  double value = 0.0;
  std::string segment;
  std::string split_test_group;
  CreativeDaypartList dayparts;
  base::flat_set<std::string> geo_targets;
  GURL target_url;
};

using CreativeAdList = std::vector<CreativeAdInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_AD_INFO_H_
