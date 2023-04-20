/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/notification_ads/notification_ads_per_day_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/notification_ad_features.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::notification_ads {

class BraveAdsNotificationAdsPerDayPermissionRuleTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    const std::vector<base::test::FeatureRefAndParams> enabled_features;

    const std::vector<base::test::FeatureRef> disabled_features;

    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                      disabled_features);
  }

  AdsPerDayPermissionRule permission_rule_;
};

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange
  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 /*count*/ kMaximumAdsPerDay.Get() - 1);

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 /*count*/ kMaximumAdsPerDay.Get());

  // Act
  AdvanceClockBy(base::Days(1));

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       DoNotAllowAdIfExceedsCapWithin1Day) {
  // Arrange
  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 /*count*/ kMaximumAdsPerDay.Get());

  // Act
  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

}  // namespace brave_ads::notification_ads
