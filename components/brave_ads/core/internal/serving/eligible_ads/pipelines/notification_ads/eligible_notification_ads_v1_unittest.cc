/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v1.h"

#include <memory>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing_random_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEligibleNotificationAdsV1Test : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    eligible_ads_ = std::make_unique<EligibleNotificationAdsV1>(
        *subdivision_targeting_, *anti_targeting_resource_);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  std::unique_ptr<EligibleNotificationAdsV1> eligible_ads_;
};

TEST_F(BraveAdsEligibleNotificationAdsV1Test, GetAdsForChildSegment) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback, Run(CreativeNotificationAdList{creative_ad_2}));
  eligible_ads_->GetForUserModel(
      UserModelInfo{
          IntentUserModelInfo{}, LatentInterestUserModelInfo{},
          InterestUserModelInfo{SegmentList{"technology & computing-software"},
                                TextEmbeddingHtmlEventList{}}},
      callback.Get());
}

TEST_F(BraveAdsEligibleNotificationAdsV1Test, GetAdsForParentSegment) {
  // Arrange
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad.segment = "technology & computing";
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback, Run(CreativeNotificationAdList{creative_ad}));
  eligible_ads_->GetForUserModel(
      UserModelInfo{
          IntentUserModelInfo{}, LatentInterestUserModelInfo{},
          InterestUserModelInfo{SegmentList{"technology & computing-software"},
                                TextEmbeddingHtmlEventList{}}},
      callback.Get());
}

TEST_F(BraveAdsEligibleNotificationAdsV1Test, GetAdsForUntargetedSegment) {
  // Arrange
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback, Run(CreativeNotificationAdList{creative_ad}));
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"finance-banking"},
                                          TextEmbeddingHtmlEventList{}}},
      callback.Get());
}

TEST_F(BraveAdsEligibleNotificationAdsV1Test, GetAdsForMultipleSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.segment = "finance-banking";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_3.segment = "food & drink";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback,
              Run(::testing::UnorderedElementsAreArray(
                  CreativeNotificationAdList{creative_ad_1, creative_ad_3})));
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{
                        SegmentList{"technology & computing", "food & drink"},
                        TextEmbeddingHtmlEventList{}}},
      callback.Get());
}

TEST_F(BraveAdsEligibleNotificationAdsV1Test, GetAdsForNoSegments) {
  // Arrange
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback, Run(CreativeNotificationAdList{creative_ad}));
  eligible_ads_->GetForUserModel(/*user_model=*/{}, callback.Get());
}

TEST_F(BraveAdsEligibleNotificationAdsV1Test, DoNotGetAdsForUnmatchedSegments) {
  // Arrange
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad.segment = "technology & computing";
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback, Run(/*creative_ads=*/::testing::IsEmpty()));
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"UNMATCHED"},
                                          TextEmbeddingHtmlEventList{}}},
      callback.Get());
}

TEST_F(BraveAdsEligibleNotificationAdsV1Test, DoNotGetAdsIfNoEligibleAds) {
  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback, Run(/*creative_ads=*/::testing::IsEmpty()));
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{
                        SegmentList{"technology & computing", "food & drink"},
                        TextEmbeddingHtmlEventList{}}},
      callback.Get());
}

TEST_F(BraveAdsEligibleNotificationAdsV1Test, DoNotGetAdsIfAlreadySeen) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  const NotificationAdInfo ad = BuildNotificationAd(creative_ad_1);
  ClientStateManager::GetInstance().UpdateSeenAd(ad);

  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback, Run(CreativeNotificationAdList{creative_ad_2}));
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{
                        SegmentList{"technology & computing", "food & drink"},
                        TextEmbeddingHtmlEventList{}}},
      callback.Get());
}

TEST_F(BraveAdsEligibleNotificationAdsV1Test, DoNotGetPacedAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";
  creative_ad_1.pass_through_rate = 0.1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.segment = "food & drink";
  creative_ad_2.pass_through_rate = 0.5;
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  const ScopedPacingRandomNumberSetterForTesting scoped_setter(0.3);

  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback, Run(CreativeNotificationAdList{creative_ad_2}));
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{
                        SegmentList{"technology & computing", "food & drink"},
                        TextEmbeddingHtmlEventList{}}},
      callback.Get());
}

TEST_F(BraveAdsEligibleNotificationAdsV1Test, GetPrioritizedAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";
  creative_ad_1.priority = 1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.segment = "finance-banking";
  creative_ad_2.priority = 1;
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_3.segment = "food & drink";
  creative_ad_3.priority = 2;
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<EligibleAdsCallback<CreativeNotificationAdList>> callback;
  EXPECT_CALL(callback, Run(CreativeNotificationAdList{creative_ad_1}));
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{
                        SegmentList{"technology & computing", "food & drink"},
                        TextEmbeddingHtmlEventList{}}},
      callback.Get());
}

}  // namespace brave_ads
