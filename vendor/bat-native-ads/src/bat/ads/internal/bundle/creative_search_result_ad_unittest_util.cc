/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_search_result_ad_unittest_util.h"

#include "bat/ads/internal/bundle/creative_ad_unittest_util.h"
#include "bat/ads/internal/bundle/creative_search_result_ad_info.h"
#include "bat/ads/internal/database/tables/creative_search_result_ads_database_table.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace ads {

void SaveCreativeAds(const CreativeSearchResultAdList& creative_ads) {
  database::table::CreativeSearchResultAds database_table;
  database_table.Save(creative_ads,
                      [](const bool success) { ASSERT_TRUE(success); });
}

CreativeSearchResultAdList BuildCreativeSearchResultAds(const int count) {
  CreativeSearchResultAdList creative_ads;

  for (int i = 0; i < count; i++) {
    const CreativeSearchResultAdInfo& creative_ad =
        BuildCreativeSearchResultAd();
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

CreativeSearchResultAdInfo BuildCreativeSearchResultAd() {
  const CreativeAdInfo& creative_ad = BuildCreativeAd();
  CreativeSearchResultAdInfo creative_search_result_ad(creative_ad);

  creative_search_result_ad.title = "Test Ad Title";
  creative_search_result_ad.body = "Test Ad Body";

  return creative_search_result_ad;
}

}  // namespace ads
