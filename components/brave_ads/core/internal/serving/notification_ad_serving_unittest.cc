/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class NotificationAdServingDelegateForTesting
    : public NotificationAdServingDelegate {
 public:
  const NotificationAdInfo& ad() const { return ad_; }

  bool opportunity_arose_to_serve_ad() const {
    return opportunity_arose_to_serve_ad_;
  }

  bool did_serve_ad() const { return did_serve_ad_; }

  bool failed_to_serve_ad() const { return failed_to_serve_ad_; }

 private:
  // NotificationAdServingDelegate:
  void OnOpportunityAroseToServeNotificationAd(
      const SegmentList& /*segments*/) override {
    opportunity_arose_to_serve_ad_ = true;
  }

  void OnDidServeNotificationAd(const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeNotificationAd() override { failed_to_serve_ad_ = true; }

  NotificationAdInfo ad_;
  bool opportunity_arose_to_serve_ad_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};

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
    ad_serving.SetDelegate(&ad_serving_delegate_);

    ad_serving.MaybeServeAd();
  }

  NotificationAdServingDelegateForTesting ad_serving_delegate_;
};

TEST_F(BraveAdsNotificationAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdServingFeature, {{"version", "0"}});

  ForcePermissionRulesForTesting();

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  MaybeServeAd();

  // Assert
  EXPECT_FALSE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
  EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
  EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
}

TEST_F(BraveAdsNotificationAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  MaybeServeAd();

  // Assert
  EXPECT_TRUE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
  EXPECT_TRUE(ad_serving_delegate_.did_serve_ad());
  EXPECT_FALSE(ad_serving_delegate_.failed_to_serve_ad());
  EXPECT_EQ(
      BuildNotificationAd(creative_ad, ad_serving_delegate_.ad().placement_id),
      ad_serving_delegate_.ad());
}

TEST_F(BraveAdsNotificationAdServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  ForcePermissionRulesForTesting();

  // Act
  MaybeServeAd();

  // Assert
  EXPECT_TRUE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
  EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
  EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
}

TEST_F(BraveAdsNotificationAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  MaybeServeAd();

  // Assert
  EXPECT_FALSE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
  EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
  EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
}

}  // namespace brave_ads
