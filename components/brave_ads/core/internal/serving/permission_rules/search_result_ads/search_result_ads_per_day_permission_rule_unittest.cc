/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/search_result_ads/search_result_ads_per_day_permission_rule.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSearchResultAdsPerDayPermissionRuleTest : public test::TestBase {
};

TEST_F(BraveAdsSearchResultAdsPerDayPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasSearchResultAdsPerDayPermission());
}

TEST_F(BraveAdsSearchResultAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_day", "3"}});

  const SearchResultAdInfo ad =
      test::BuildSearchResultAd(/*should_generate_random_uuids=*/true);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumSearchResultAdsPerDay.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasSearchResultAdsPerDayPermission());
}

TEST_F(BraveAdsSearchResultAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Day) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_day", "3"}});

  const SearchResultAdInfo ad =
      test::BuildSearchResultAd(/*should_generate_random_uuids=*/true);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumSearchResultAdsPerDay.Get());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(HasSearchResultAdsPerDayPermission());
}

TEST_F(BraveAdsSearchResultAdsPerDayPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Day) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_day", "3"}});

  const SearchResultAdInfo ad =
      test::BuildSearchResultAd(/*should_generate_random_uuids=*/true);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumSearchResultAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasSearchResultAdsPerDayPermission());
}

TEST_F(BraveAdsSearchResultAdsPerDayPermissionRuleTest, ShouldAlwaysAllow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_day", "0"}});

  // Act & Assert
  EXPECT_TRUE(HasSearchResultAdsPerDayPermission());
}

}  // namespace brave_ads
