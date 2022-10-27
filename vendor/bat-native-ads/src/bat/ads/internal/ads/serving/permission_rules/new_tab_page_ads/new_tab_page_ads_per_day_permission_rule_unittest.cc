/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/new_tab_page_ads/new_tab_page_ads_per_day_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::new_tab_page_ads {

class BatAdsNewTabPageAdsPerDayPermissionRuleTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
        enabled_features;

    const std::vector<base::test::FeatureRef> disabled_features;

    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                      disabled_features);
  }
};

TEST_F(BatAdsNewTabPageAdsPerDayPermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  AdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerDayPermissionRuleTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  const int count = features::GetMaximumNewTabPageAdsPerDay() - 1;
  RecordAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed, count);

  // Act
  AdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerDayPermissionRuleTest,
       AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  const int count = features::GetMaximumNewTabPageAdsPerDay();
  RecordAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed, count);

  AdvanceClockBy(base::Days(1));

  // Act
  AdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerDayPermissionRuleTest,
       DoNotAllowAdIfExceedsCapWithin1Day) {
  // Arrange
  const int count = features::GetMaximumNewTabPageAdsPerDay();
  RecordAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed, count);

  AdvanceClockBy(base::Days(1) - base::Seconds(1));

  // Act
  AdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads::new_tab_page_ads
