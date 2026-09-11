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

TEST_F(BraveAdsCreativeAdRoundRobinTest, UnservedAdsShouldBeEligible) {
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

  creative_ad_round_robin_.MarkAsServed(creative_ad_1);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_2, creative_ad_3));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       AllAdsShouldBeEligibleAgainAfterEachHasBeenServed) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2};

  creative_ad_round_robin_.MarkAsServed(creative_ad_1);
  creative_ad_round_robin_.MarkAsServed(creative_ad_2);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert: the rotation resets once every ad has been served, so both ads
  // are immediately eligible again, including the one served last.
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1, creative_ad_2));
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
  creative_ad_round_robin_.MarkAsServed(stale_creative_ad);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       AllAdsShouldBeEligibleWhenNoneHaveBeenServed) {
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
       AllAdsShouldBeEligibleWhenLastServedIsStaleAndAllHaveBeenServed) {
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

  creative_ad_round_robin_.MarkAsServed(creative_ad_1);
  creative_ad_round_robin_.MarkAsServed(creative_ad_2);
  creative_ad_round_robin_.MarkAsServed(stale_creative_ad);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1, creative_ad_2));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       ShouldNotFilterAdsWhenRoundRobinIsDisabled) {
  // Arrange
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      kEligibleAdFeature, {{"should_round_robin", "false"}});

  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_round_robin_.MarkAsServed(creative_ad_1);

  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);

  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2};

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1, creative_ad_2));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       ShouldKeepUnservedAdEligibleWhenOtherAdHasBeenServed) {
  // Arrange
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_round_robin_.MarkAsServed(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);

  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2};

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_2));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       AdShouldBeEligibleAgainWhenItIsTheOnlyAd) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  CreativeNewTabPageAdList creative_ads = {creative_ad};

  creative_ad_round_robin_.MarkAsServed(creative_ad);

  // Act
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       FilteringEmptyAdsShouldNotAffectServedState) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_round_robin_.MarkAsServed(creative_ad_1);

  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);

  CreativeNewTabPageAdList empty_creative_ads;

  // Act: filtering an empty list is a no-op and should not reset the
  // priority bucket's served state.
  creative_ad_round_robin_.Filter(empty_creative_ads);

  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2};
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_2));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       PriorityBucketsShouldRotateIndependently) {
  // Arrange
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_round_robin_.MarkAsServed(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  CreativeNewTabPageAdList other_priority_creative_ads = {creative_ad_2};

  // Act: filtering a different priority bucket should be unaffected by
  // `creative_ad_1`'s priority bucket having been fully rotated.
  creative_ad_round_robin_.Filter(other_priority_creative_ads);

  // Assert
  EXPECT_THAT(other_priority_creative_ads, testing::ElementsAre(creative_ad_2));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest, FilteringTwiceShouldBeIdempotent) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_round_robin_.MarkAsServed(creative_ad_1);

  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2};

  // Act
  creative_ad_round_robin_.Filter(creative_ads);
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_2));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       MarkAsServedShouldCreateNewPriorityBucketOnDemand) {
  // Arrange
  CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad.priority = 3;

  // Act
  creative_ad_round_robin_.MarkAsServed(creative_ad);

  CreativeNewTabPageAdList creative_ads = {creative_ad};
  creative_ad_round_robin_.Filter(creative_ads);

  // Assert: the bucket for priority 3 was created and reset because the only
  // ad in it has now been served.
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad));
}

TEST_F(BraveAdsCreativeAdRoundRobinTest,
       BucketShouldResetOnEveryRotationNotJustTheFirst) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);

  // Act & Assert: first rotation.
  creative_ad_round_robin_.MarkAsServed(creative_ad_1);
  creative_ad_round_robin_.MarkAsServed(creative_ad_2);

  CreativeNewTabPageAdList creative_ads = {creative_ad_1, creative_ad_2};
  creative_ad_round_robin_.Filter(creative_ads);
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1, creative_ad_2));

  // Act & Assert: second rotation.
  creative_ad_round_robin_.MarkAsServed(creative_ad_1);
  creative_ad_round_robin_.MarkAsServed(creative_ad_2);

  creative_ads = {creative_ad_1, creative_ad_2};
  creative_ad_round_robin_.Filter(creative_ads);
  EXPECT_THAT(creative_ads, testing::ElementsAre(creative_ad_1, creative_ad_2));
}

}  // namespace brave_ads
