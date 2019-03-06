/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_CREATIVE_SET_INFO_H_
#define BAT_ADS_INTERNAL_CATALOG_CREATIVE_SET_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/catalog_segment_info.h"
#include "bat/ads/internal/catalog_creative_info.h"

namespace ads {

struct CreativeSetInfo {
  CreativeSetInfo();
  explicit CreativeSetInfo(const std::string& creative_set_id);
  CreativeSetInfo(const CreativeSetInfo& info);
  ~CreativeSetInfo();

  std::string creative_set_id;
  std::string execution;
  unsigned int per_day;
  unsigned int total_max;
  std::vector<SegmentInfo> segments;
  std::vector<CreativeInfo> creatives;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_CREATIVE_SET_INFO_H_
