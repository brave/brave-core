/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"

namespace ads {

CreativePromotedContentAdInfo::CreativePromotedContentAdInfo() = default;

CreativePromotedContentAdInfo::CreativePromotedContentAdInfo(
    const CreativeAdInfo& creative_ad)
    : CreativeAdInfo(creative_ad) {}

bool CreativePromotedContentAdInfo::operator==(
    const CreativePromotedContentAdInfo& other) const {
  return CreativeAdInfo::operator==(other) && title == other.title &&
         description == other.description;
}

bool CreativePromotedContentAdInfo::operator!=(
    const CreativePromotedContentAdInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
