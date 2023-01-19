/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/creatives/creative_ad_info.h"

namespace ads {

struct CreativePromotedContentAdInfo final : CreativeAdInfo {
  CreativePromotedContentAdInfo();
  explicit CreativePromotedContentAdInfo(const CreativeAdInfo& creative_ad);

  bool operator==(const CreativePromotedContentAdInfo& other) const;
  bool operator!=(const CreativePromotedContentAdInfo& other) const;

  std::string title;
  std::string description;
};

using CreativePromotedContentAdList =
    std::vector<CreativePromotedContentAdInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_
