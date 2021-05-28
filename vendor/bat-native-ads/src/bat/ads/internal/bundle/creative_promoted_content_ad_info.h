/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

struct CreativePromotedContentAdInfo : CreativeAdInfo {
  CreativePromotedContentAdInfo();
  ~CreativePromotedContentAdInfo();

  bool operator==(const CreativePromotedContentAdInfo& rhs) const;

  bool operator!=(const CreativePromotedContentAdInfo& rhs) const;

  std::string title;
  std::string description;
};

using CreativePromotedContentAdList =
    std::vector<CreativePromotedContentAdInfo>;

CreativeAdList ToCreativeAdList(
    const CreativePromotedContentAdList& creative_promoted_content_ads) {
  CreativeAdList creative_ads;

  for (const auto& creative_promoted_content_ad :
       creative_promoted_content_ads) {
    const CreativeAdInfo creative_ad =
        static_cast<CreativeAdInfo>(creative_promoted_content_ad);
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_
