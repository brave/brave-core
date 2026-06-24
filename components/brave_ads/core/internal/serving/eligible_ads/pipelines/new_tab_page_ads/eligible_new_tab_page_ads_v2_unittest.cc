/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v2.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/test/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing_random_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/round_robin/creative_ad_round_robin.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/test/ad_event_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEligibleNewTabPageAdsV2Test : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kEligibleAdFeature, {{"should_round_robin", "true"}});

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    eligible_ads_ = std::make_unique<EligibleNewTabPageAdsV2>(
        *subdivision_targeting_, *anti_targeting_resource_,
        creative_ad_round_robin_);
  }

  void SimulateServeAd(const CreativeNewTabPageAdInfo& creative_ad) {
    creative_ad_round_robin_.MarkAsServed(creative_ad);
  }

  void SimulateServeAd(const CreativeNewTabPageAdList& creative_ads) {
    SimulateServeAd(creative_ads.at(0));
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  CreativeAdRoundRobin creative_ad_round_robin_;
  std::unique_ptr<EligibleNewTabPageAdsV2> eligible_ads_;
};

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, GetAds) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.segment = "parent";

  CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_3.segment = "parent-child";

  test::SaveCreativeNewTabPageAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  // Act & Assert
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, GetAdsForNoMatchingSegments) {
  // Arrange
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.segment = "parent";

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.segment = "parent-child";

  test::SaveCreativeNewTabPageAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(/*user_model=*/{}, test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::IsEmpty());
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, DoNotGetAdsForExpiredCampaign) {
  // Arrange
  CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*should_generate_random_uuids=*/true);
  creative_ad.end_at = test::Now() - base::Days(1);
  test::SaveCreativeNewTabPageAds({creative_ad});

  // Act & Assert
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::IsEmpty());
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, DoNotGetAdsForFutureCampaign) {
  // Arrange
  CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*should_generate_random_uuids=*/true);
  creative_ad.start_at = test::Now() + base::Days(1);
  creative_ad.end_at = test::DistantFuture();
  test::SaveCreativeNewTabPageAds({creative_ad});

  // Act & Assert
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::IsEmpty());
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, DoNotGetAdsIfNoEligibleAds) {
  // Act & Assert
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{
          IntentUserModelInfo{SegmentList{"parent-child", "parent"}},
          LatentInterestUserModelInfo{},
          InterestUserModelInfo{SegmentList{"parent-child", "parent"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::IsEmpty());
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, RoundRobinAlwaysServesASingleAd) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  test::SaveCreativeNewTabPageAds({creative_ad});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act & Assert: the same ad is served every time because it is the only ad.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad));
  }

  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad));
  }
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       RoundRobinServesEachAdOnceBeforeRepeating) {
  // Arrange
  {
    CreativeNewTabPageAdList creative_ads;
    creative_ads.push_back(test::BuildCreativeNewTabPageAd(
        CreativeNewTabPageAdWallpaperType::kImage,
        /*use_random_uuids=*/true));
    creative_ads.push_back(test::BuildCreativeNewTabPageAd(
        CreativeNewTabPageAdWallpaperType::kImage,
        /*use_random_uuids=*/true));
    test::SaveCreativeNewTabPageAds(creative_ads);
  }

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act
  base::test::TestFuture<CreativeNewTabPageAdList> test_future_1;
  eligible_ads_->GetForUserModel(user_model, test_future_1.GetCallback());
  const CreativeNewTabPageAdList creative_ads_1 = test_future_1.Take();
  EXPECT_THAT(creative_ads_1, ::testing::SizeIs(1));
  SimulateServeAd(creative_ads_1);

  base::test::TestFuture<CreativeNewTabPageAdList> test_future_2;
  eligible_ads_->GetForUserModel(user_model, test_future_2.GetCallback());
  const CreativeNewTabPageAdList creative_ads_2 = test_future_2.Take();
  EXPECT_THAT(creative_ads_2, ::testing::SizeIs(1));
  SimulateServeAd(creative_ads_2);

  ASSERT_NE(creative_ads_1, creative_ads_2);

  // Assert: `creative_ads_1` is served again because `creative_ads_2` was
  // served last and is not served immediately after all ads have been seen.
  base::test::TestFuture<CreativeNewTabPageAdList> test_future_3;
  eligible_ads_->GetForUserModel(user_model, test_future_3.GetCallback());
  EXPECT_EQ(test_future_3.Take(), creative_ads_1);
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, ZeroPriorityAdIsNeverServed) {
  // Arrange: `creative_ad_1` has priority 0 and must be excluded by the
  // bucketing step; `creative_ad_2` has priority 1 and should be served.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 0;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 1;

  test::SaveCreativeNewTabPageAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
}

TEST_F(
    BraveAdsEligibleNewTabPageAdsV2Test,
    HigherPriorityAdIsServedBeforeLowerPriorityAdWithNonSequentialPriorities) {
  // Arrange: `creative_ad_1` has priority 1 and `creative_ad_2` has priority 5.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 5;

  test::SaveCreativeNewTabPageAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       HigherPriorityAdsAreServedBeforeLowerPriorityAds) {
  // Arrange: `creative_ad_1` has priority 1 so it must be served over
  // `creative_ad_2` which has priority 2.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  test::SaveCreativeNewTabPageAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       HigherPriorityAdContinuesToServeAfterRoundRobinReset) {
  // Arrange: `creative_ad_1` has priority 1 and `creative_ad_2` has priority 2.
  // After the only priority 1 ad is served, its per-bucket rotation resets
  // so it remains eligible. The lower-priority `creative_ad_2` must never be
  // served as long as the higher-priority bucket has an eligible ad.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  test::SaveCreativeNewTabPageAds({creative_ad_1, creative_ad_2});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act: `creative_ad_1` is served as it has the highest priority.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
    SimulateServeAd(creative_ad_1);
  }

  // Assert: the priority 1 bucket rotation resets (it is the only ad), so
  // `creative_ad_1` serves again rather than falling through to
  // `creative_ad_2`.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       RoundRobinResetsWithinPriorityBucketAfterAllAdsHaveBeenShown) {
  // Arrange: `creative_ad_1` and `creative_ad_2` share a priority 1;
  // `creative_ad_3` has a priority 2 and must never be served while the
  // priority 1 bucket has eligible ads.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 1;

  CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_3.priority = 2;

  test::SaveCreativeNewTabPageAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act: serve each priority 1 ad once.
  base::test::TestFuture<CreativeNewTabPageAdList> test_future_1;
  eligible_ads_->GetForUserModel(user_model, test_future_1.GetCallback());
  const CreativeNewTabPageAdList first_served_creative_ads =
      test_future_1.Take();
  EXPECT_THAT(first_served_creative_ads, ::testing::SizeIs(1));
  SimulateServeAd(first_served_creative_ads);

  base::test::TestFuture<CreativeNewTabPageAdList> test_future_2;
  eligible_ads_->GetForUserModel(user_model, test_future_2.GetCallback());
  const CreativeNewTabPageAdList second_served_creative_ads =
      test_future_2.Take();
  EXPECT_THAT(second_served_creative_ads, ::testing::SizeIs(1));
  ASSERT_NE(first_served_creative_ads, second_served_creative_ads);
  SimulateServeAd(second_served_creative_ads);

  // Assert: all priority 1 ads have been seen so the bucket resets.
  // `second_served_creative_ads` is excluded; `first_served_creative_ads` comes
  // back. The lower-priority `creative_ad_3` is never reached.
  base::test::TestFuture<CreativeNewTabPageAdList> test_future_3;
  eligible_ads_->GetForUserModel(user_model, test_future_3.GetCallback());
  EXPECT_EQ(test_future_3.Take(), first_served_creative_ads);
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       RoundRobinPrefersNewHigherPriorityAdOverLowerPriorityAd) {
  // Arrange: `creative_ad_1` has priority 1 and `creative_ad_2` has priority 2.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  test::SaveCreativeNewTabPageAds({creative_ad_1, creative_ad_2});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act: serve `creative_ad_1` (priority 1).
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
    SimulateServeAd(creative_ad_1);
  }

  // Add a new priority 1 ad to the campaign.
  CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_3.priority = 1;
  test::SaveCreativeNewTabPageAds({creative_ad_3});

  // Assert: `creative_ad_3` is the only unseen ad in the priority 1 bucket
  // (round-robin excludes the already-seen `creative_ad_1`), so it is served
  // ahead of the lower-priority `creative_ad_2`.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
  }
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       LowerPriorityAdIsServedWhenHigherPriorityAdHitsItsFrequencyCap) {
  // Arrange: `creative_ad_1` has priority 1 with a frequency cap of 1;
  // `creative_ad_2` has priority 2. Once `creative_ad_1` has been served once
  // today its frequency cap is exhausted and the pipeline must fall through to
  // `creative_ad_2`.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  test::SaveCreativeNewTabPageAds({creative_ad_1, creative_ad_2});

  test::RecordAdEvents(BuildNewTabPageAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Act & Assert: `creative_ad_1` has exhausted its frequency cap so the
  // pipeline falls through to the lower-priority `creative_ad_2`.
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       LowerPriorityAdIsServedWhenAllHigherPriorityAdsHitTheirFrequencyCaps) {
  // Arrange
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 1;
  creative_ad_2.per_day = 1;

  CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_3.priority = 2;

  test::SaveCreativeNewTabPageAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  test::RecordAdEvents(BuildNewTabPageAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);
  test::RecordAdEvents(BuildNewTabPageAd(creative_ad_2),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Act & Assert: all priority 1 ads are frequency-capped so the pipeline
  // falls through to `creative_ad_3`.
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       EachLowerPriorityAdIsServedWhenHigherPriorityAdsAreFrequencyCapped) {
  // Arrange: `creative_ad_1` has priority 1, `creative_ad_2` has priority 2,
  // and `creative_ad_3` has priority 3. Each higher-priority ad has a frequency
  // cap of 1 so the pipeline cascades through each bucket in order.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 2;
  creative_ad_2.per_day = 1;

  CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_3.priority = 3;

  test::SaveCreativeNewTabPageAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act & Assert: priority 1 ad is served first.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }

  test::RecordAdEvents(BuildNewTabPageAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Priority 1 is capped; priority 2 ad is served next.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
  }

  test::RecordAdEvents(BuildNewTabPageAd(creative_ad_2),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Both priority 1 and 2 are capped; priority 3 ad is served last.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
  }
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       UncappedHigherPriorityAdServesWhenOtherHigherPriorityAdsAreCapped) {
  // Arrange: `creative_ad_1` is frequency-capped; `creative_ad_2` shares
  // priority 1 but is not capped. The pipeline must serve `creative_ad_2`
  // rather than falling through to `creative_ad_3` (priority 2).
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 1;

  CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_3.priority = 2;

  test::SaveCreativeNewTabPageAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  test::RecordAdEvents(BuildNewTabPageAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Act & Assert: `creative_ad_2` is eligible in the priority 1 bucket so
  // the pipeline serves it rather than falling through to `creative_ad_3`.
  base::test::TestFuture<CreativeNewTabPageAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       HigherPriorityAdResumesServingAfterFrequencyCapResets) {
  // Arrange: `creative_ad_1` (priority 1) exhausts its frequency cap so the
  // pipeline falls through to `creative_ad_2` (priority 2). After a day the
  // cap resets and `creative_ad_1` must be served again.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  test::SaveCreativeNewTabPageAds({creative_ad_1, creative_ad_2});

  test::RecordAdEvents(BuildNewTabPageAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Confirm the cap is active and `creative_ad_2` is served as fallback.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
  }

  // Advance past the one-day frequency cap window.
  AdvanceClockBy(base::Days(1));

  // Assert: `creative_ad_1`'s cap has reset so the pipeline serves it again
  // ahead of the lower-priority `creative_ad_2`.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }
}

TEST_F(
    BraveAdsEligibleNewTabPageAdsV2Test,
    HigherPriorityBucketTakesPrecedenceImmediatelyAfterCapResetsWhileServingLowerPriority) {
  // Arrange: `creative_ad_1` (priority 1) is frequency-capped so the pipeline
  // falls through to the priority 2 bucket which has two ads mid-rotation.
  // When the priority 1 cap resets, the next serve must jump back to
  // `creative_ad_1` rather than continuing the lower-priority rotation.
  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_3.priority = 2;

  test::SaveCreativeNewTabPageAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  test::RecordAdEvents(BuildNewTabPageAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Serve from the lower-priority bucket while `creative_ad_1` is capped.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    const CreativeNewTabPageAdList lower_priority_served_creative_ads =
        test_future.Take();
    EXPECT_THAT(lower_priority_served_creative_ads, ::testing::SizeIs(1));
    SimulateServeAd(lower_priority_served_creative_ads);
  }

  // Advance past the one-day frequency cap window so `creative_ad_1` is
  // eligible again.
  AdvanceClockBy(base::Days(1));

  // Assert: the priority 1 bucket has an eligible ad again so
  // `creative_ad_1` is served immediately, even though the priority 2
  // rotation is mid-cycle.
  {
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test,
       LowPtrCampaignDoesNotBlockHighPtrCampaignInSamePriorityBucket) {
  // Arrange: an ad passes pacing when roll < `pass_through_rate` and is paced
  // out when roll >= `pass_through_rate`.

  // Campaign 1 (priority 1): two ads with a `pass_through_rate` of 0.01.
  const std::string campaign_1_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();

  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_1.campaign_id = campaign_1_id;
  creative_ad_1.priority = 1;
  creative_ad_1.pass_through_rate = 0.01;

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_2.campaign_id = campaign_1_id;
  creative_ad_2.priority = 1;
  creative_ad_2.pass_through_rate = 0.01;

  // Campaign 2 (priority 1): one ad with a `pass_through_rate` of 0.42.
  CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_3.priority = 1;
  creative_ad_3.pass_through_rate = 0.42;

  // Campaign 3 (priority 2): one ad with a `pass_through_rate` of 1.0.
  CreativeNewTabPageAdInfo creative_ad_4 =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  creative_ad_4.priority = 2;
  creative_ad_4.pass_through_rate = 1.0;

  test::SaveCreativeNewTabPageAds(
      {creative_ad_1, creative_ad_2, creative_ad_3, creative_ad_4});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // NTT 1: cold start (no prior served state). Roll of 0.005 passes all three
  // priority 1 ads. The round-robin starts fresh with no exclusions, so any
  // priority 1 ad is eligible. `creative_ad_4` (priority 2) is not
  // eligible while priority 1 has eligible ads. No ad is marked as served so
  // the round-robin state remains empty for the regression sequence below.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.005);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(
        test_future.Take(),
        ::testing::AllOf(::testing::SizeIs(1),
                         ::testing::Not(::testing::Contains(creative_ad_4))));
  }

  // NTT 2: roll of 0.2 paces campaign 1 out (its `pass_through_rate`
  // of 0.01, 0.2 >= 0.01). Campaign 2's ad passes pacing (its
  // `pass_through_rate` of 0.42, 0.2 < 0.42) and is served.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.2);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    ASSERT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
    SimulateServeAd(creative_ad_3);
  }

  // NTT 3: roll of 0.005 is below campaign 1's `pass_through_rate` (0.01) so
  // all three priority 1 ads pass pacing. The round-robin removes
  // `creative_ad_3` (already served), leaving `[creative_ad_1, creative_ad_2]`.
  // `creative_ad_4` (priority 2) is not eligible while priority 1 has eligible
  // ads. A campaign 1 ad is eligible.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.005);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    const CreativeNewTabPageAdList creative_ads = test_future.Take();
    EXPECT_THAT(creative_ads,
                ::testing::AllOf(::testing::SizeIs(1),
                                 ::testing::Not(::testing::AnyOf(
                                     ::testing::Contains(creative_ad_3),
                                     ::testing::Contains(creative_ad_4)))));
    SimulateServeAd(creative_ads);
  }

  // NTT 4: roll of 0.2 paces campaign 1 out again. Pacing produces
  // `[creative_ad_3]`. The round-robin sees that all candidates in the
  // post-pacing bucket have been served and resets, making `creative_ad_3`
  // eligible. `creative_ad_3` serves.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.2);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
    SimulateServeAd(creative_ad_3);
  }

  // NTT 5: roll of 0.2 again; the same reset-and-serve pattern from NTT 4
  // repeats. Pacing produces `[creative_ad_3]`, the round-robin resets on the
  // single-element bucket, and `creative_ad_3` serves.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.2);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
    SimulateServeAd(creative_ad_3);
  }

  // NTT 6: all three priority 1 ads pass pacing. The round-robin removes
  // `creative_ad_3` (served in NTT 5), leaving `[creative_ad_1,
  // creative_ad_2]`. `creative_ad_4` (priority 2) is not eligible. A campaign 1
  // ad serves, confirming campaign 1 is not permanently blocked.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.005);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    const CreativeNewTabPageAdList creative_ads = test_future.Take();
    EXPECT_THAT(creative_ads,
                ::testing::AllOf(::testing::SizeIs(1),
                                 ::testing::Not(::testing::AnyOf(
                                     ::testing::Contains(creative_ad_3),
                                     ::testing::Contains(creative_ad_4)))));
    SimulateServeAd(creative_ads);
  }

  // NTT 7: roll of 0.2 paces campaign 1 out; only `creative_ad_3` passes
  // pacing. The round-robin resets because `creative_ad_3` is the only
  // candidate in the post-pacing bucket and has already been served, starting
  // the second rotation.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.2);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
    SimulateServeAd(creative_ad_3);
  }

  // NTT 8: campaign 1 resumes in the second rotation. `creative_ad_3` is
  // excluded by the round-robin (last served in NTT 7); `creative_ad_4`
  // (priority 2) is not eligible. A campaign 1 ad serves.
  CreativeNewTabPageAdList ntt8_creative_ads;
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.005);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    ntt8_creative_ads = test_future.Take();
    EXPECT_THAT(ntt8_creative_ads,
                ::testing::AllOf(::testing::SizeIs(1),
                                 ::testing::Not(::testing::AnyOf(
                                     ::testing::Contains(creative_ad_3),
                                     ::testing::Contains(creative_ad_4)))));
    SimulateServeAd(ntt8_creative_ads);
  }

  // NTT 9: the other campaign 1 ad serves, confirming the full three-ad cycle
  // repeats correctly after the reset. `creative_ad_4` (priority 2) is not
  // eligible while priority 1 has eligible ads.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.005);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    const CreativeNewTabPageAdList creative_ads = test_future.Take();
    EXPECT_THAT(creative_ads,
                ::testing::AllOf(::testing::SizeIs(1),
                                 ::testing::Not(::testing::AnyOf(
                                     ::testing::Contains(creative_ad_3),
                                     ::testing::Contains(creative_ad_4)))));
    EXPECT_NE(ntt8_creative_ads, creative_ads);
    SimulateServeAd(creative_ads);
  }

  // NTT 10: roll of 0.5 paces out all priority 1 ads; campaign 1 has a
  // `pass_through_rate` of 0.01 and campaign 2 a `pass_through_rate` of 0.42.
  // The priority 1 bucket is empty so the pipeline falls through to the
  // priority 2 bucket where `creative_ad_4` (a `pass_through_rate` of 1.0)
  // serves.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.5);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_4));
    SimulateServeAd(creative_ad_4);
  }

  // NTT 11: priority 1 immediately resumes serving as soon as it has an
  // eligible ad. Roll of 0.2 paces campaign 1 out but campaign 2 passes
  // pacing (`pass_through_rate` of 0.42, 0.2 < 0.42). `creative_ad_3` serves
  // from priority 1 rather than `creative_ad_4` from priority 2.
  {
    const ScopedPacingRandomNumberSetterForTesting scoped_pacing_random_number(
        0.2);
    base::test::TestFuture<CreativeNewTabPageAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
  }
}

}  // namespace brave_ads
