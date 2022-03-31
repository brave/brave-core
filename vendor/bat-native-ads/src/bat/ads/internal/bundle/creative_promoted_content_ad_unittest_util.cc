/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_promoted_content_ad_unittest_util.h"

#include "bat/ads/internal/bundle/creative_ad_unittest_util.h"
#include "bat/ads/internal/bundle/creative_promoted_content_ad_info.h"
#include "bat/ads/internal/database/tables/creative_promoted_content_ads_database_table.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace ads {

void SaveCreativeAds(const CreativePromotedContentAdList& creative_ads) {
  database::table::CreativePromotedContentAds database_table;
  database_table.Save(creative_ads,
                      [](const bool success) { ASSERT_TRUE(success); });
}

CreativePromotedContentAdList BuildCreativePromotedContentAds(const int count) {
  CreativePromotedContentAdList creative_ads;

  for (int i = 0; i < count; i++) {
    const CreativePromotedContentAdInfo& creative_ad =
        BuildCreativePromotedContentAd();
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

CreativePromotedContentAdInfo BuildCreativePromotedContentAd() {
  const CreativeAdInfo& creative_ad = BuildCreativeAd();
  CreativePromotedContentAdInfo creative_promoted_content_ad(creative_ad);

  creative_promoted_content_ad.title = "Test Ad Title";
  creative_promoted_content_ad.description = "Test Ad Description";

  return creative_promoted_content_ad;
}

}  // namespace ads
