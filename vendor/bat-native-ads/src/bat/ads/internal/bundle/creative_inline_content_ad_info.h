/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_INLINE_CONTENT_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_INLINE_CONTENT_AD_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

struct CreativeInlineContentAdInfo : CreativeAdInfo {
  CreativeInlineContentAdInfo();
  CreativeInlineContentAdInfo(const CreativeInlineContentAdInfo& info);
  ~CreativeInlineContentAdInfo();

  bool operator==(const CreativeInlineContentAdInfo& rhs) const;

  bool operator!=(const CreativeInlineContentAdInfo& rhs) const;

  std::string title;
  std::string description;
  std::string image_url;
  std::string dimensions;
  std::string cta_text;
};

using CreativeInlineContentAdList = std::vector<CreativeInlineContentAdInfo>;

CreativeAdList ToCreativeAdList(
    const CreativeInlineContentAdList& creative_inline_content_ads) {
  CreativeAdList creative_ads;

  for (const auto& creative_inline_content_ad : creative_inline_content_ads) {
    const CreativeAdInfo creative_ad =
        static_cast<CreativeAdInfo>(creative_inline_content_ad);
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_INLINE_CONTENT_AD_INFO_H_
