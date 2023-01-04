/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_AD_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_AD_UNITTEST_UTIL_H_

#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"

namespace ads {

void SaveCreativeAds(const CreativePromotedContentAdList& creative_ads);

CreativePromotedContentAdList BuildCreativePromotedContentAds(int count);
CreativePromotedContentAdInfo BuildCreativePromotedContentAd(
    bool should_use_random_guids = true);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_AD_UNITTEST_UTIL_H_
