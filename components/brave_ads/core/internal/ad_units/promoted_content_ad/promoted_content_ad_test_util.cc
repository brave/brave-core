/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_test_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"

namespace brave_ads::test {

PromotedContentAdInfo BuildPromotedContentAd(
    const bool should_generate_random_uuids) {
  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAd(should_generate_random_uuids);
  return BuildPromotedContentAd(creative_ad);
}

PromotedContentAdInfo BuildAndSavePromotedContentAd(
    const bool should_generate_random_uuids) {
  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAd(should_generate_random_uuids);
  database::SaveCreativePromotedContentAds({creative_ad});
  return BuildPromotedContentAd(creative_ad);
}

}  // namespace brave_ads::test
