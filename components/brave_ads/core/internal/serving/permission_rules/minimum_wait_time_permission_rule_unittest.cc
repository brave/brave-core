/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/minimum_wait_time_permission_rule.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsMinimumWaitTimePermissionRuleTest : public test::TestBase {};

TEST_F(BraveAdsMinimumWaitTimePermissionRuleTest, ShouldAllowIfNoHistory) {
  // Arrange
  const std::vector<base::Time> history;

  // Act & Assert
  EXPECT_TRUE(HasMinimumWaitTimePermission(
      history, /*time_constraint=*/base::Minutes(1)));
}

TEST_F(BraveAdsMinimumWaitTimePermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  const std::vector<base::Time> history = {test::Now()};

  AdvanceClockBy(base::Minutes(1));

  // Act & Assert
  EXPECT_TRUE(HasMinimumWaitTimePermission(
      history, /*time_constraint=*/base::Minutes(1)));
}

TEST_F(BraveAdsMinimumWaitTimePermissionRuleTest, ShouldNotAllowIfExceedsCap) {
  // Arrange
  const std::vector<base::Time> history = {test::Now()};

  AdvanceClockBy(base::Minutes(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasMinimumWaitTimePermission(
      history, /*time_constraint=*/base::Minutes(1)));
}

}  // namespace brave_ads
