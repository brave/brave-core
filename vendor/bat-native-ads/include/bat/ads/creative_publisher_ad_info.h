/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CREATIVE_PUBLISHER_AD_INFO_H_
#define BAT_ADS_CREATIVE_PUBLISHER_AD_INFO_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ads/creative_ad_info.h"
#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT CreativePublisherAdInfo : CreativeAdInfo {
  CreativePublisherAdInfo();
  CreativePublisherAdInfo(
      const CreativePublisherAdInfo& info);
  ~CreativePublisherAdInfo();

  std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::string size;
  std::string creative_url;
  std::vector<std::string> channels;
};

using CreativePublisherAdList = std::vector<CreativePublisherAdInfo>;
using CreativePublisherAdMap = std::map<std::string, CreativePublisherAdList>;

}  // namespace ads

#endif  // BAT_ADS_CREATIVE_PUBLISHER_AD_INFO_H_
