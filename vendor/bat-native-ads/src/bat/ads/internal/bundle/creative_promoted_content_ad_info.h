/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_

#include <string>

#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

struct CreativePromotedContentAdInfo final : CreativeAdInfo {
  CreativePromotedContentAdInfo();
  CreativePromotedContentAdInfo(const CreativePromotedContentAdInfo& info);
  ~CreativePromotedContentAdInfo();

  bool operator==(const CreativePromotedContentAdInfo& rhs) const;
  bool operator!=(const CreativePromotedContentAdInfo& rhs) const;

  std::string title;
  std::string description;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_
