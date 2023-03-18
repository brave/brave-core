/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/notification_ads/notification_ads_per_day_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_features.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::notification_ads {

class BatAdsNotificationAdsPerDayPermissionRuleTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    const std::vector<base::test::FeatureRefAndParams> enabled_features;

    const std::vector<base::test::FeatureRef> disabled_features;

    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                      disabled_features);
  }
};

TEST_F(BatAdsNotificationAdsPerDayPermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  AdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsPerDayPermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange
  const size_t count = features::GetMaximumAdsPerDay() - 1;
  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed, count);

  // Act
  AdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsPerDayPermissionRuleTest,
       AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  const size_t count = features::GetMaximumAdsPerDay();
  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed, count);

  AdvanceClockBy(base::Days(1));

  // Act
  AdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsPerDayPermissionRuleTest,
       DoNotAllowAdIfExceedsCapWithin1Day) {
  // Arrange
  const size_t count = features::GetMaximumAdsPerDay();
  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed, count);

  AdvanceClockBy(base::Days(1) - base::Seconds(1));

  // Act
  AdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace brave_ads::notification_ads
