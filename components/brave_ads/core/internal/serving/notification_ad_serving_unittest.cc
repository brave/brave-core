/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdServingTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    NotifyDidInitializeAds();
  }

  void MaybeServeAd() {
    SubdivisionTargeting subdivision_targeting;
    AntiTargetingResource anti_targeting_resource;
    NotificationAdServing ad_serving(subdivision_targeting,
                                     anti_targeting_resource);
    ad_serving.SetDelegate(&delegate_mock_);

    ad_serving.MaybeServeAd();
  }

  ::testing::StrictMock<NotificationAdServingDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsNotificationAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdServingFeature, {{"version", "0"}});

  test::ForcePermissionRules();

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToServeNotificationAd);
  MaybeServeAd();
}

TEST_F(BraveAdsNotificationAdServingTest, ServeAd) {
  // Arrange
  test::ForcePermissionRules();

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnOpportunityAroseToServeNotificationAd);
  EXPECT_CALL(delegate_mock_, OnDidServeNotificationAd);
  MaybeServeAd();
}

TEST_F(BraveAdsNotificationAdServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  test::ForcePermissionRules();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnOpportunityAroseToServeNotificationAd);
  EXPECT_CALL(delegate_mock_, OnFailedToServeNotificationAd);
  MaybeServeAd();
}

TEST_F(BraveAdsNotificationAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToServeNotificationAd);
  MaybeServeAd();
}

}  // namespace brave_ads
