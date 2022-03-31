/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_INLINE_CONTENT_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_INLINE_CONTENT_AD_INFO_H_

#include <string>

#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

struct CreativeInlineContentAdInfo final : CreativeAdInfo {
  CreativeInlineContentAdInfo();
  CreativeInlineContentAdInfo(const CreativeInlineContentAdInfo& info);
  explicit CreativeInlineContentAdInfo(const CreativeAdInfo& creative_ad);
  ~CreativeInlineContentAdInfo();

  bool operator==(const CreativeInlineContentAdInfo& rhs) const;
  bool operator!=(const CreativeInlineContentAdInfo& rhs) const;

  std::string title;
  std::string description;
  std::string image_url;
  std::string dimensions;
  std::string cta_text;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_INLINE_CONTENT_AD_INFO_H_
