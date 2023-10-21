/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/new_tab_page_ads/new_tab_page_ads_minimum_wait_time_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest
    : public UnitTestBase {
 protected:
  const NewTabPageAdMinimumWaitTimePermissionRule permission_rule_;
};

TEST_F(BraveAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  test::RecordAdEvent(AdType::kNewTabPageAd, ConfirmationType::kServed);

  AdvanceClockBy(kNewTabPageAdMinimumWaitTime.Get());

  // Act & Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest,
       ShouldNotAllowIfExceedsCap) {
  // Arrange
  test::RecordAdEvent(AdType::kNewTabPageAd, ConfirmationType::kServed);

  AdvanceClockBy(kNewTabPageAdMinimumWaitTime.Get() - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

}  // namespace brave_ads
