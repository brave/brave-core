/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_features_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_observer.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::notification_ads {

class BatAdsNotificationAdServingTest : public ServingObserver,
                                        public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    ForceServingVersion(1);

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    serving_ = std::make_unique<Serving>(*subdivision_targeting_,
                                         *anti_targeting_resource_);
    serving_->AddObserver(this);
  }

  void TearDown() override {
    serving_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnOpportunityAroseToServeNotificationAd(
      const SegmentList& /*segments*/) override {
    had_opportunuity_ = true;
  }

  void OnDidServeNotificationAd(const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeNotificationAd() override { failed_to_serve_ad_ = true; }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<Serving> serving_;

  NotificationAdInfo ad_;
  bool had_opportunuity_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};

TEST_F(BatAdsNotificationAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  ForceServingVersion(0);

  // Act
  serving_->MaybeServeAd();

  // Assert
  EXPECT_FALSE(had_opportunuity_);
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(failed_to_serve_ad_);
}

TEST_F(BatAdsNotificationAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  SaveCreativeAds({creative_ad});

  // Act
  serving_->MaybeServeAd();

  // Assert
  EXPECT_TRUE(had_opportunuity_);
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_FALSE(failed_to_serve_ad_);

  const NotificationAdInfo expected_ad =
      BuildNotificationAd(creative_ad, ad_.placement_id);
  EXPECT_EQ(expected_ad, ad_);
}

TEST_F(BatAdsNotificationAdServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  ForcePermissionRulesForTesting();

  // Act
  serving_->MaybeServeAd();

  // Assert
  EXPECT_FALSE(had_opportunuity_);
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(failed_to_serve_ad_);
}

TEST_F(BatAdsNotificationAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  SaveCreativeAds({creative_ad});

  // Act
  serving_->MaybeServeAd();

  // Assert
  EXPECT_FALSE(had_opportunuity_);
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(failed_to_serve_ad_);
}

}  // namespace brave_ads::notification_ads
