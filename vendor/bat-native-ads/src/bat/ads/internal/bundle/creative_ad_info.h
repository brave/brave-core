/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_AD_INFO_H_

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/time/time.h"
#include "bat/ads/internal/bundle/creative_daypart_info.h"

namespace ads {

struct CreativeAdInfo {
  CreativeAdInfo();
  CreativeAdInfo(const CreativeAdInfo& info);
  ~CreativeAdInfo();

  bool operator==(const CreativeAdInfo& rhs) const;
  bool operator!=(const CreativeAdInfo& rhs) const;

  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  base::Time start_at;
  base::Time end_at;
  int daily_cap = 0;
  int priority = 0;
  double ptr = 0.0;
  bool conversion = false;
  int per_day = 0;
  int per_week = 0;
  int per_month = 0;
  int total_max = 0;
  double value = 0.0;
  std::string segment;
  std::string split_test_group;
  std::vector<CreativeDaypartInfo> dayparts;
  base::flat_set<std::string> geo_targets;
  std::string target_url;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_AD_INFO_H_
