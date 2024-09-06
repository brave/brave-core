/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/new_tab_page_ads/new_tab_page_ads_per_day_permission_rule.h"

#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdsPerDayPermissionRuleTest : public test::TestBase {};

TEST_F(BraveAdsNewTabPageAdsPerDayPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasNewTabPageAdsPerDayPermission());
}

TEST_F(BraveAdsNewTabPageAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildNewTabPageAd(/*should_generate_random_uuids=*/false);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumNewTabPageAdsPerDay.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasNewTabPageAdsPerDayPermission());
}

TEST_F(BraveAdsNewTabPageAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Day) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildNewTabPageAd(/*should_generate_random_uuids=*/false);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumNewTabPageAdsPerDay.Get());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(HasNewTabPageAdsPerDayPermission());
}

TEST_F(BraveAdsNewTabPageAdsPerDayPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Day) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildNewTabPageAd(/*should_generate_random_uuids=*/false);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumNewTabPageAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasNewTabPageAdsPerDayPermission());
}

}  // namespace brave_ads
