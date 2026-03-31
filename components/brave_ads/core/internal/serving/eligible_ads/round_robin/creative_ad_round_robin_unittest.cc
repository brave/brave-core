/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/round_robin/creative_ad_round_robin.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/test/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr char kStaleCreativeInstanceId[] =
    "deadbeef-dead-4ead-8ead-deadbeefcafe";
}  // namespace

class BraveAdsCreativeAdRoundRobinTest : public testing::Test {
 protected:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kEligibleAdFeature, {{"should_round_robin", "true"}});
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  CreativeAdRoundRobin creative_ad_round_robin_;
};

TEST_F(BraveAdsCreativeAdRoundRobinTest, UnseenAdsShouldBeEligible) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2,
                                           creative_ad_3};

  creative_ad_round_robin_.MarkAsSeen(creative_ad_1);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_2, creative_ad_3));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       AllAdsShouldBeEligibleAgainAfterEachHasBeenSeen) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2};

  creative_ad_round_robin_.MarkAsSeen(creative_ad_1);
  creative_ad_round_robin_.MarkAsSeen(creative_ad_2);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert: `creative_ad_2` was last seen so it is excluded from the next
  // rotation to avoid an immediate repeat.
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       LastSeenAdShouldBeExcludedWhenAllAdsHaveBeenShown) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2,
                                           creative_ad_3};

  creative_ad_round_robin_.MarkAsSeen(creative_ad_1);
  creative_ad_round_robin_.MarkAsSeen(creative_ad_2);
  creative_ad_round_robin_.MarkAsSeen(creative_ad_3);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1, creative_ad_2));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       LastSeenAdShouldBeEligibleWhenItIsTheOnlyAd) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  CreativeNewTabPageAdList creative_ads = {creative_ad};

  creative_ad_round_robin_.MarkAsSeen(creative_ad);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       StaleCreativeInstanceIdsShouldNotAffectCreativeAdsEligibility) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  CreativeNewTabPageAdList creative_ads = {creative_ad};

  CreativeNewTabPageAdInfo stale_creative_ad;
  stale_creative_ad.creative_instance_id = kStaleCreativeInstanceId;
  creative_ad_round_robin_.MarkAsSeen(stale_creative_ad);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       AllAdsShouldBeEligibleAfterFilteringEmptyList) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);

  CreativeNewTabPageAdInfo stale_creative_ad;
  stale_creative_ad.creative_instance_id = kStaleCreativeInstanceId;
  creative_ad_round_robin_.MarkAsSeen(stale_creative_ad);

  CreativeNewTabPageAdList empty_ads;
  creative_ad_round_robin_.Filter(empty_ads);

  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2};

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1, creative_ad_2));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       AllAdsShouldBeEligibleWhenNoneHaveBeenSeen) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2,
                                           creative_ad_3};

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1, creative_ad_2,
                                                 creative_ad_3));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       AllAdsShouldBeEligibleWhenLastSeenIsStaleAndAllHaveBeenShown) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2};

  CreativeNewTabPageAdInfo stale_creative_ad;
  stale_creative_ad.creative_instance_id = kStaleCreativeInstanceId;

  creative_ad_round_robin_.MarkAsSeen(creative_ad_1);
  creative_ad_round_robin_.MarkAsSeen(creative_ad_2);
  creative_ad_round_robin_.MarkAsSeen(stale_creative_ad);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1, creative_ad_2));
}

}  // namespace brave_ads
