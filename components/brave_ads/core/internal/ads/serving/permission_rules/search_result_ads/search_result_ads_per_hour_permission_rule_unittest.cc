/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/search_result_ads/search_result_ads_per_hour_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/search_result_ad_features.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::search_result_ads {

class BatAdsSearchResultAdsPerHourPermissionRuleTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    const std::vector<base::test::FeatureRefAndParams> enabled_features;

    const std::vector<base::test::FeatureRef> disabled_features;

    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                      disabled_features);
  }

  AdsPerHourPermissionRule permission_rule_;
};

TEST_F(BatAdsSearchResultAdsPerHourPermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsSearchResultAdsPerHourPermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange

  // Act
  RecordAdEvents(AdType::kSearchResultAd, ConfirmationType::kServed,
                 /*count*/ kMaximumAdsPerHour.Get() - 1);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsSearchResultAdsPerHourPermissionRuleTest,
       AllowAdIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  RecordAdEvents(AdType::kSearchResultAd, ConfirmationType::kServed,
                 /*count*/ kMaximumAdsPerHour.Get());

  // Act
  AdvanceClockBy(base::Hours(1));

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsSearchResultAdsPerHourPermissionRuleTest,
       DoNotAllowAdIfExceedsCapWithin1Hour) {
  // Arrange
  RecordAdEvents(AdType::kSearchResultAd, ConfirmationType::kServed,
                 /*count*/ kMaximumAdsPerHour.Get());

  // Act
  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

}  // namespace brave_ads::search_result_ads
