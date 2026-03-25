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
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

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
        *subdivision_targeting_, *anti_targeting_resource_);
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
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
  const CreativeNotificationAdList creative_ads_1 = test_future_1.Take();
  EXPECT_THAT(creative_ads_1, ::testing::SizeIs(1));

  base::test::TestFuture<CreativeNotificationAdList> test_future_2;
  eligible_ads_->GetForUserModel(user_model, test_future_2.GetCallback());
  const CreativeNotificationAdList creative_ads_2 = test_future_2.Take();
  EXPECT_THAT(creative_ads_2, ::testing::SizeIs(1));

  ASSERT_NE(creative_ads_1, creative_ads_2);

  // Assert: `creative_ads_1` is served again because `creative_ads_2` was
  // served last and is not served immediately after all ads have been seen.
  base::test::TestFuture<CreativeNotificationAdList> test_future_3;
  eligible_ads_->GetForUserModel(user_model, test_future_3.GetCallback());
  EXPECT_EQ(test_future_3.Take(), creative_ads_1);
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test, ZeroPriorityAdIsNeverServed) {
  // Arrange: `creative_ad_1` has priority 0 and must be excluded by the
  // bucketing step; `creative_ad_2` has the default priority 2 and should be
  // served.
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.priority = 0;

  const CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<CreativeNotificationAdList> test_future;
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      test_future.GetCallback());
  EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
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
       RoundRobinServesLowerPriorityAdAfterHigherPriorityAdHasBeenShown) {
  // Arrange: `creative_ad_1` has priority 1 and `creative_ad_2` has priority 2.
  // After `creative_ad_1` is served, round-robin excludes it so the
  // higher-priority bucket is empty and the pipeline falls through to the
  // lower-priority bucket.
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
  }

  // Assert: round-robin excludes the already-seen `creative_ad_1`, leaving
  // only the lower-priority `creative_ad_2`.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
  }
}

TEST_F(BraveAdsEligibleNotificationAdsV2Test,
       RoundRobinServesHigherPriorityAdAgainAfterAllAdsHaveBeenShown) {
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

  // Act: serve each ad once so that all ads have been seen.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }

  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_2));
  }

  // Assert: all ads have been seen so the rotation resets. `creative_ad_1` is
  // served again because `creative_ad_2` is excluded as the most recently
  // served ad.
  {
    base::test::TestFuture<CreativeNotificationAdList> test_future;
    eligible_ads_->GetForUserModel(user_model, test_future.GetCallback());
    EXPECT_THAT(test_future.Take(), ::testing::ElementsAre(creative_ad_1));
  }
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

}  // namespace brave_ads
