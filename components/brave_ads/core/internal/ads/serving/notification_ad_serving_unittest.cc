/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_feature_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdServingTest : public NotificationAdServingDelegate,
                                          public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    NotifyDidInitializeAds();

    ForceNotificationAdServingVersionForTesting(1);

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    serving_ = std::make_unique<NotificationAdServing>(
        *subdivision_targeting_, *anti_targeting_resource_);
    serving_->SetDelegate(this);
  }

  void OnOpportunityAroseToServeNotificationAd(
      const SegmentList& /*segments*/) override {
    opportunity_arose_to_serve_ad_ = true;
  }

  void OnDidServeNotificationAd(const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeNotificationAd() override { failed_to_serve_ad_ = true; }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  std::unique_ptr<NotificationAdServing> serving_;

  NotificationAdInfo ad_;
  bool opportunity_arose_to_serve_ad_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};

TEST_F(BraveAdsNotificationAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  ForceNotificationAdServingVersionForTesting(0);

  // Act
  serving_->MaybeServeAd();

  // Assert
  EXPECT_FALSE(opportunity_arose_to_serve_ad_);
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(failed_to_serve_ad_);
}

TEST_F(BraveAdsNotificationAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  serving_->MaybeServeAd();

  // Assert
  EXPECT_TRUE(opportunity_arose_to_serve_ad_);
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_FALSE(failed_to_serve_ad_);
  EXPECT_EQ(BuildNotificationAd(creative_ad, ad_.placement_id), ad_);
}

TEST_F(BraveAdsNotificationAdServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  ForcePermissionRulesForTesting();

  // Act
  serving_->MaybeServeAd();

  // Assert
  EXPECT_FALSE(opportunity_arose_to_serve_ad_);
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(failed_to_serve_ad_);
}

TEST_F(BraveAdsNotificationAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  serving_->MaybeServeAd();

  // Assert
  EXPECT_FALSE(opportunity_arose_to_serve_ad_);
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(failed_to_serve_ad_);
}

}  // namespace brave_ads
