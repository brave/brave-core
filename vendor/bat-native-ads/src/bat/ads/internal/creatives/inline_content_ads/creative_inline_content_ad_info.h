/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_AD_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "url/gurl.h"

namespace ads {

struct CreativeInlineContentAdInfo final : CreativeAdInfo {
  CreativeInlineContentAdInfo();
  explicit CreativeInlineContentAdInfo(const CreativeAdInfo& creative_ad);

  CreativeInlineContentAdInfo(const CreativeInlineContentAdInfo& other);
  CreativeInlineContentAdInfo& operator=(
      const CreativeInlineContentAdInfo& other);

  CreativeInlineContentAdInfo(CreativeInlineContentAdInfo&& other) noexcept;
  CreativeInlineContentAdInfo& operator=(
      CreativeInlineContentAdInfo&& other) noexcept;

  ~CreativeInlineContentAdInfo();

  bool operator==(const CreativeInlineContentAdInfo& other) const;
  bool operator!=(const CreativeInlineContentAdInfo& other) const;

  std::string title;
  std::string description;
  GURL image_url;
  std::string dimensions;
  std::string cta_text;
};

using CreativeInlineContentAdList = std::vector<CreativeInlineContentAdInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_AD_INFO_H_
