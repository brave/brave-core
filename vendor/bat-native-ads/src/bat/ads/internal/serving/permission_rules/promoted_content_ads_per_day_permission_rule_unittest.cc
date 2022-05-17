/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/promoted_content_ads_per_day_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/serving/serving_features.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsPromotedContentAdsPerDayPermissionRuleTest : public UnitTestBase {
 protected:
  BatAdsPromotedContentAdsPerDayPermissionRuleTest() = default;

  ~BatAdsPromotedContentAdsPerDayPermissionRuleTest() override = default;

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

TEST_F(BatAdsPromotedContentAdsPerDayPermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  PromotedContentAdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsPromotedContentAdsPerDayPermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange
  const int count = features::GetMaximumPromotedContentAdsPerDay() - 1;
  RecordAdEvents(AdType::kPromotedContentAd, ConfirmationType::kServed, count);

  // Act
  PromotedContentAdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsPromotedContentAdsPerDayPermissionRuleTest,
       AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  const int count = features::GetMaximumPromotedContentAdsPerDay();
  RecordAdEvents(AdType::kPromotedContentAd, ConfirmationType::kServed, count);

  FastForwardClockBy(base::Days(1));

  // Act
  PromotedContentAdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsPromotedContentAdsPerDayPermissionRuleTest,
       DoNotAllowAdIfExceedsCapWithin1Day) {
  // Arrange
  const int count = features::GetMaximumPromotedContentAdsPerDay();
  RecordAdEvents(AdType::kPromotedContentAd, ConfirmationType::kServed, count);

  FastForwardClockBy(base::Hours(23));

  // Act
  PromotedContentAdsPerDayPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
