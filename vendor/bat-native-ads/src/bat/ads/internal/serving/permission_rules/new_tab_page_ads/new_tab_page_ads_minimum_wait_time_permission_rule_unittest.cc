/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/new_tab_page_ads/new_tab_page_ads_minimum_wait_time_permission_rule.h"

#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/serving/serving_features.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace new_tab_page_ads {

class BatAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest
    : public UnitTestBase {
 protected:
  BatAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest() = default;

  ~BatAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest() override = default;
};

TEST_F(BatAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  MinimumWaitTimePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange
  RecordAdEvent(AdType::kNewTabPageAd, ConfirmationType::kServed);

  AdvanceClockBy(features::GetNewTabPageAdsMinimumWaitTime());

  // Act
  MinimumWaitTimePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsMinimumWaitTimePermissionRuleTest,
       DoNotAllowAdIfExceedsCap) {
  // Arrange
  RecordAdEvent(AdType::kNewTabPageAd, ConfirmationType::kServed);

  AdvanceClockBy(features::GetNewTabPageAdsMinimumWaitTime() -
                 base::Seconds(1));

  // Act
  MinimumWaitTimePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace new_tab_page_ads
}  // namespace ads
