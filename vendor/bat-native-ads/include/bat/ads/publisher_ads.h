/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_PUBLISHER_ADS_H_
#define BAT_ADS_PUBLISHER_ADS_H_

#include <string>
#include <vector>

#include "bat/ads/export.h"
#include "bat/ads/result.h"
#include "bat/ads/publisher_ad_info.h"

namespace ads {

struct ADS_EXPORT PublisherAds {
  PublisherAds();
  PublisherAds(
      const PublisherAds& info);
  ~PublisherAds();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::vector<PublisherAdInfo> entries;
};

}  // namespace ads

#endif  // BAT_ADS_PUBLISHER_ADS_H_
