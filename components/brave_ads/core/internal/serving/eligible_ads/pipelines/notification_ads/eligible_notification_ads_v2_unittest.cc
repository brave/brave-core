/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v2.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/test/creative_notification_ad_test_util.h"
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

class BraveAdsEligibleNotificationAdsV2Test : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kEligibleAdFeature, {{"should_round_robin", "true"}});

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    eligible_ads_ = std::make_unique<EligibleNotificationAdsV2>(
        *subdivision_targeting_, *anti_targeting_resource_,
        creative_ad_round_robin_);
  }

  void SimulateServeAd(const CreativeNotificationAdInfo& creative_ad) {
    creative_ad_round_robin_.MarkAsServed(creative_ad);
  }

  void SimulateServeAd(const CreativeNotificationAdList& creative_ads) {
    SimulateServeAd(creative_ads.at(0));
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  CreativeAdRoundRobin creative_ad_round_robin_;
  std::unique_ptr<EligibleNotificationAdsV2> eligible_ads_;
};

TEST_F(BraveAdsEligibleNotificationAdsV2Test, GetAds) {
  // Arrange
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.segment = "parent-child-1";

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.segment = "parent-child-3";

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{
          IntentUserModelInfo{SegmentList{"parent-child-1", "parent-child-2"}},
          LatentInterestUserModelInfo{},
          InterestUserModelInfo{SegmentList{"parent-child-3"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::SizeIs(1));
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test, GetAdsForNoMatchingSegments) {
  // Arrange
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.segment = "parent";

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.segment = "parent-child";

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(/*user_model=*/{}, test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::IsEmpty());
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test, DoNotGetAdsForExpiredCampaign) {
  // Arrange
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad.end_at = test::Now() - base::Days(1);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(/*user_model=*/{}, test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::IsEmpty());
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test, DoNotGetAdsForFutureCampaign) {
  // Arrange
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad.start_at = test::Now() + base::Days(1);
  creative_ad.end_at = test::DistantFuture();
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(/*user_model=*/{}, test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::IsEmpty());
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test, DoNotGetAdsIfNoEligibleAds) {
  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{
          IntentUserModelInfo{SegmentList{"parent-child", "parent"}},
          LatentInterestUserModelInfo{},
          InterestUserModelInfo{SegmentList{"parent-child", "parent"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::IsEmpty());
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test, RoundRobinAlwaysServesASingleAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  database::SaveCreativeNotificationAds({creative_ad});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act & Assert: the same ad is served every time because it is the only ad.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad));
  }

  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad));
  }
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       RoundRobinServesEachAdOnceBeforeRepeating) {
  // Arrange
  {
    CreativeNotificationAdList creative_ads;
    creative_ads.push_back(
        test::BuildCreativeNotificationAd(/*use_random_uuids=*/true));
    creative_ads.push_back(
        test::BuildCreativeNotificationAd(/*use_random_uuids=*/true));
    database::SaveCreativeNotificationAds(creative_ads);
  }

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act
  base::test::TestFuture<CreativeNotificationAdList> test_future_1;
  eligible_ads_->GetForUserModel(user_model, test_future_1.GetCallback());
  const CreativeNotificationAdList first_served_creative_ads =
      test_future_1.Take();
  EXPECT_THAT(first_served_creative_ads, ::testing::SizeIs(1));
  SimulateServeAd(first_served_creative_ads);

  base::test::TestFuture<CreativeNotificationAdList> test_future_2;
  eligible_ads_->GetForUserModel(user_model, test_future_2.GetCallback());
  const CreativeNotificationAdList second_served_creative_ads =
      test_future_2.Take();
  EXPECT_THAT(second_served_creative_ads, ::testing::SizeIs(1));
  SimulateServeAd(second_served_creative_ads);

  ASSERT_NE(first_served_creative_ads, second_served_creative_ads);

  // Assert: `first_served_creative_ads` is served again because
  // `second_served_creative_ads` was served last and is not served immediately
  // after all ads have been seen.
  base::test::TestFuture<CreativeNotificationAdList> test_future_3;
  eligible_ads_->GetForUserModel(user_model, test_future_3.GetCallback());
  EXPECT_EQ(test_future_3.Take(), first_served_creative_ads);
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test, ZeroPriorityAdIsNeverServed) {
  // Arrange: `creative_ad_1` has priority 0 and must be excluded by the
  // bucketing step; `creative_ad_2` has priority 1 and should be served.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 0;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 1;

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
}

TEST_F(
    BraveAdsEligibleNotificationAdsV2Test,
    HigherPriorityAdIsServedBeforeLowerPriorityAdWithNonSequentialPriorities) {
  // Arrange: `creative_ad_1` has priority 1 and `creative_ad_2` has priority 5.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 5;

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       HigherPriorityAdsAreServedBeforeLowerPriorityAds) {
  // Arrange: `creative_ad_1` has priority 1 so it must be served over
  // `creative_ad_2` which has priority 2.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       HigherPriorityAdContinuesToServeAfterRoundRobinReset) {
  // Arrange: `creative_ad_1` has priority 1 and `creative_ad_2` has priority 2.
  // After the only priority 1 ad is served, its per-bucket rotation resets
  // so it remains eligible. The lower-priority `creative_ad_2` must never be
  // served as long as the higher-priority bucket has an eligible ad.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act: `creative_ad_1` is served as it has the highest priority.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
    SimulateServeAd(creative_ad_1);
  }

  // Assert: the priority 1 bucket rotation resets (it is the only ad), so
  // `creative_ad_1` serves again rather than falling through to
  // `creative_ad_2`.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       RoundRobinResetsWithinPriorityBucketAfterAllAdsHaveBeenShown) {
  // Arrange: `creative_ad_1` and `creative_ad_2` share a priority 1;
  // `creative_ad_3` has a priority 2 and must never be served while the
  // priority 1 bucket has eligible ads.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 1;

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_3.priority = 2;

  database::SaveCreativeNotificationAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act: serve each priority 1 ad once.
  base::test::TestFuture<CreativeNotificationAdList> test_future_1;
  eligible_ads_->GetForUserModel(user_model, test_future_1.GetCallback());
  const CreativeNotificationAdList first_served_creative_ads =
      test_future_1.Take();
  EXPECT_THAT(first_served_creative_ads, ::testing::SizeIs(1));
  SimulateServeAd(first_served_creative_ads);

  base::test::TestFuture<CreativeNotificationAdList> test_future_2;
  eligible_ads_->GetForUserModel(user_model, test_future_2.GetCallback());
  const CreativeNotificationAdList second_served_creative_ads =
      test_future_2.Take();
  EXPECT_THAT(second_served_creative_ads, ::testing::SizeIs(1));
  ASSERT_NE(first_served_creative_ads, second_served_creative_ads);
  SimulateServeAd(second_served_creative_ads);

  // Assert: all priority 1 ads have been seen so the bucket resets.
  // The second-served ad is excluded; the first-served ad comes back.
  // The lower-priority `creative_ad_3` is never reached.
  base::test::TestFuture<CreativeNotificationAdList> test_future_3;
  eligible_ads_->GetForUserModel(user_model, test_future_3.GetCallback());
  EXPECT_EQ(test_future_3.Take(), first_served_creative_ads);
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       RoundRobinPrefersNewHigherPriorityAdOverLowerPriorityAd) {
  // Arrange: `creative_ad_1` has priority 1 and `creative_ad_2` has priority 2.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act: serve `creative_ad_1` (priority 1).
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
    SimulateServeAd(creative_ad_1);
  }

  // Add a new priority 1 ad to the campaign.
  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_3.priority = 1;
  database::SaveCreativeNotificationAds({creative_ad_3});

  // Assert: `creative_ad_3` is the only unseen ad in the priority 1 bucket
  // (round-robin excludes the already-seen `creative_ad_1`), so it is served
  // ahead of the lower-priority `creative_ad_2`.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
  }
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       LowerPriorityAdIsServedWhenHigherPriorityAdHitsItsFrequencyCap) {
  // Arrange: `creative_ad_1` has priority 1 with a frequency cap of 1;
  // `creative_ad_2` has priority 2. Once `creative_ad_1` has been served once
  // today its frequency cap is exhausted and the pipeline must fall through to
  // `creative_ad_2`.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  test::RecordAdEvents(BuildNotificationAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Act & Assert: `creative_ad_1` has exhausted its frequency cap so the
  // pipeline falls through to the lower-priority `creative_ad_2`.
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       LowerPriorityAdIsServedWhenAllHigherPriorityAdsHitTheirFrequencyCaps) {
  // Arrange: both priority 1 ads have exhausted their frequency caps;
  // `creative_ad_3` has priority 2 and must be served as a fallback.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 1;
  creative_ad_2.per_day = 1;

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_3.priority = 2;

  database::SaveCreativeNotificationAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  test::RecordAdEvents(BuildNotificationAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);
  test::RecordAdEvents(BuildNotificationAd(creative_ad_2),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Act & Assert: all priority 1 ads are frequency-capped so the pipeline
  // falls through to `creative_ad_3`.
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       EachLowerPriorityAdIsServedWhenHigherPriorityAdsAreFrequencyCapped) {
  // Arrange: `creative_ad_1` has priority 1, `creative_ad_2` has priority 2,
  // and `creative_ad_3` has priority 3. Each higher-priority ad has a frequency
  // cap of 1 so the pipeline cascades through each bucket in order.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 2;
  creative_ad_2.per_day = 1;

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_3.priority = 3;

  database::SaveCreativeNotificationAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Act & Assert: priority 1 ad is served first.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }

  test::RecordAdEvents(BuildNotificationAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Priority 1 is capped; priority 2 ad is served next.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
  }

  test::RecordAdEvents(BuildNotificationAd(creative_ad_2),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Both priority 1 and 2 are capped; priority 3 ad is served last.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_3));
  }
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       UncappedHigherPriorityAdServesWhenOtherHigherPriorityAdsAreCapped) {
  // Arrange: `creative_ad_1` is frequency-capped; `creative_ad_2` shares
  // priority 1 but is not capped. The pipeline must serve `creative_ad_2`
  // rather than falling through to `creative_ad_3` (priority 2).
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 1;

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_3.priority = 2;

  database::SaveCreativeNotificationAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  test::RecordAdEvents(BuildNotificationAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  // Act & Assert: `creative_ad_2` is eligible in the priority 1 bucket so
  // the pipeline serves it rather than falling through to `creative_ad_3`.
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       HigherPriorityAdResumesServingAfterFrequencyCapResets) {
  // Arrange: `creative_ad_1` (priority 1) exhausts its frequency cap so the
  // pipeline falls through to `creative_ad_2` (priority 2). After a day the
  // cap resets and `creative_ad_1` must be served again.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  test::RecordAdEvents(BuildNotificationAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Confirm the cap is active and `creative_ad_2` is served as fallback.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
  }

  // Advance past the one-day frequency cap window.
  AdvanceClockBy(base::Days(1));

  // Assert: `creative_ad_1`'s cap has reset so the pipeline serves it again
  // ahead of the lower-priority `creative_ad_2`.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }
}

TEST_F(
    BraveAdsEligibleNotificationAdsV2Test,
    HigherPriorityBucketTakesPrecedenceImmediatelyAfterCapResetsWhileServingLowerPriority) {
  // Arrange: `creative_ad_1` (priority 1) is frequency-capped so the pipeline
  // falls through to the priority 2 bucket which has two ads mid-rotation.
  // When the priority 1 cap resets, the next serve must jump back to
  // `creative_ad_1` rather than continuing the lower-priority rotation.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.per_day = 1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 2;

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_3.priority = 2;

  database::SaveCreativeNotificationAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  test::RecordAdEvents(BuildNotificationAd(creative_ad_1),
                       mojom::ConfirmationType::kServedImpression,
                       /*count=*/1);

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{SegmentList{"untargeted"}}};

  // Serve from the lower-priority bucket while `creative_ad_1` is capped.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    const CreativeNotificationAdList lower_priority_served_creative_ads =
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
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       FallThroughToLowerPriorityBucketWhenPacingDrainsHigherPriorityBucket) {
  // Arrange: `creative_ad_1` (priority 1) has a zero pass-through rate so
  // pacing always removes it, leaving an empty bucket. The pipeline must fall
  // through to `creative_ad_2` (priority 2).
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ad_1.pass_through_rate = 0.0;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.priority = 2;
  creative_ad_2.pass_through_rate = 1.0;

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  const ScopedPacingRandomNumberSetterForTesting scoped_setter(0.5);

  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
}

}  // namespace brave_ads
