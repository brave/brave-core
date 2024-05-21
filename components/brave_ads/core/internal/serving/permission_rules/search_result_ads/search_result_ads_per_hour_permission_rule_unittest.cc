/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSearchResultAdsPerHourPermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsSearchResultAdsPerHourPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasSearchResultAdsPerHourPermission());
}

TEST_F(BraveAdsSearchResultAdsPerHourPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_hour", "3"}});

  test::RecordAdEvents(AdType::kSearchResultAd,
                       ConfirmationType::kServedImpression,
                       /*count=*/kMaximumSearchResultAdsPerHour.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasSearchResultAdsPerHourPermission());
}

TEST_F(BraveAdsSearchResultAdsPerHourPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_hour", "3"}});

  test::RecordAdEvents(AdType::kSearchResultAd,
                       ConfirmationType::kServedImpression,
                       /*count=*/kMaximumSearchResultAdsPerHour.Get());

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  EXPECT_TRUE(HasSearchResultAdsPerHourPermission());
}

TEST_F(BraveAdsSearchResultAdsPerHourPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Hour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_hour", "3"}});

  test::RecordAdEvents(AdType::kSearchResultAd,
                       ConfirmationType::kServedImpression,
                       /*count=*/kMaximumSearchResultAdsPerHour.Get());

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasSearchResultAdsPerHourPermission());
}

TEST_F(BraveAdsSearchResultAdsPerHourPermissionRuleTest, ShouldAlwaysAllow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_hour", "0"}});

  // Act & Assert
  EXPECT_TRUE(HasSearchResultAdsPerHourPermission());
}

}  // namespace brave_ads
