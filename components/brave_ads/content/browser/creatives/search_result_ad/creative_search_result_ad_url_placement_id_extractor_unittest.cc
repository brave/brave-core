/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_url_placement_id_extractor.h"

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeSearchResultAdUrlPlacementIdExtractorTest,
     ExtractCreativeAdPlacementIdFromUrls) {
  // Allowed domains.
  EXPECT_EQ(
      test::kCreativeAdPlacementId,
      MaybeExtractCreativeAdPlacementIdFromUrl(GURL(
          R"(https://search.brave.com/a/redirect?click_url=https://brave.com&placement_id=953f362e-98cd-4fa6-8403-e886185b88fc&creative_instance_id=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)")));

  EXPECT_EQ(
      test::kCreativeAdPlacementId,
      MaybeExtractCreativeAdPlacementIdFromUrl(GURL(
          R"(https://safesearch.brave.com/a/redirect?click_url=https://brave.com&placement_id=953f362e-98cd-4fa6-8403-e886185b88fc&creative_instance_id=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)")));

  EXPECT_EQ(
      test::kCreativeAdPlacementId,
      MaybeExtractCreativeAdPlacementIdFromUrl(GURL(
          R"(https://search.brave.software/a/redirect?click_url=https://brave.com&placement_id=953f362e-98cd-4fa6-8403-e886185b88fc&creative_instance_id=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)")));
}

TEST(BraveAdsCreativeSearchResultAdUrlPlacementIdExtractorTest,
     DoNotExtractCreativeAdPlacementIdFromUrls) {
  // Invalid URL.
  EXPECT_FALSE(MaybeExtractCreativeAdPlacementIdFromUrl(GURL()));

  // Non https://.
  EXPECT_FALSE(MaybeExtractCreativeAdPlacementIdFromUrl(
      GURL(R"(http://search.brave.software/a/redirect)")));

  // Disallowed domain.
  EXPECT_FALSE(MaybeExtractCreativeAdPlacementIdFromUrl(
      GURL(R"(https://brave.software/a/redirect)")));

  // Invalid path.
  EXPECT_FALSE(MaybeExtractCreativeAdPlacementIdFromUrl(
      GURL(R"(https://search.brave.software/foo/bar)")));

  // No query.
  EXPECT_FALSE(MaybeExtractCreativeAdPlacementIdFromUrl(
      GURL("https://search.brave.software/a/redirect?")));

  // No `placement_id` query.
  EXPECT_FALSE(MaybeExtractCreativeAdPlacementIdFromUrl(GURL(
      R"(https://search.brave.software/a/redirect?click_url=https://brave.com&creative_instance_id=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)")));

  // No `placement_id` query value.
  EXPECT_FALSE(MaybeExtractCreativeAdPlacementIdFromUrl(GURL(
      R"(https://search.brave.software/a/redirect?click_url=https://brave.com&placement_id=)")));

  EXPECT_FALSE(MaybeExtractCreativeAdPlacementIdFromUrl(GURL(
      R"(https://search.brave.software/a/redirect?click_url=https://brave.com&placement_id)")));
}

}  // namespace brave_ads
