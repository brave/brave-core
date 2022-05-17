/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/search_result_ads_per_day_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/serving/serving_features.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsSearchResultAdsPerDayPermissionRuleTest : public UnitTestBase {
 protected:
  BatAdsSearchResultAdsPerDayPermissionRuleTest() = default;

  ~BatAdsSearchResultAdsPerDayPermissionRuleTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
        enabled_features;

    const std::vector<base::Feature> disabled_features;

    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                      disabled_features);
  }
};

TEST_F(BatAdsSearchResultAdsPerDayPermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  SearchResultAdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsSearchResultAdsPerDayPermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange
  const int count = features::GetMaximumSearchResultAdsPerDay() - 1;
  RecordAdEvents(AdType::kSearchResultAd, ConfirmationType::kServed, count);

  // Act
  SearchResultAdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsSearchResultAdsPerDayPermissionRuleTest,
       AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  const int count = features::GetMaximumSearchResultAdsPerDay();
  RecordAdEvents(AdType::kSearchResultAd, ConfirmationType::kServed, count);

  FastForwardClockBy(base::Days(1));

  // Act
  SearchResultAdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsSearchResultAdsPerDayPermissionRuleTest,
       DoNotAllowAdIfExceedsCapWithin1Day) {
  // Arrange
  const int count = features::GetMaximumSearchResultAdsPerDay();
  RecordAdEvents(AdType::kSearchResultAd, ConfirmationType::kServed, count);

  FastForwardClockBy(base::Hours(23));

  // Act
  SearchResultAdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
