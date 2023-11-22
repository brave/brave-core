/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_unittest_util.h"

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_unittest_constants.h"

namespace brave_ads::test {

CreativePromotedContentAdList BuildCreativePromotedContentAds(const int count) {
  CHECK_GT(count, 0);

  CreativePromotedContentAdList creative_ads;

  for (int i = 0; i < count; ++i) {
    CreativePromotedContentAdInfo creative_ad = BuildCreativePromotedContentAd(
        /*should_use_random_uuids=*/true);
    creative_ad.segment = kSegments[i % std::size(kSegments)];

    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

CreativePromotedContentAdInfo BuildCreativePromotedContentAd(
    const bool should_use_random_uuids) {
  const CreativeAdInfo creative_ad = BuildCreativeAd(should_use_random_uuids);
  CreativePromotedContentAdInfo creative_promoted_content_ad(creative_ad);

  creative_promoted_content_ad.title = "Test Ad Title";
  creative_promoted_content_ad.description = "Test Ad Description";

  return creative_promoted_content_ad;
}

}  // namespace brave_ads::test
