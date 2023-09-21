/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTimeConstraintUtilTest : public UnitTestBase {};

TEST_F(BraveAdsTimeConstraintUtilTest, DoesRespectIfNoHistory) {
  // Arrange
  const std::vector<base::Time> history;

  // Act
  const bool does_respect = DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint*/ base::Days(1), /*cap*/ 1);

  // Assert
  EXPECT_TRUE(does_respect);
}

TEST_F(BraveAdsTimeConstraintUtilTest, DoesRespect) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());

  AdvanceClockBy(base::Days(1));

  // Act
  const bool does_respect = DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint*/ base::Days(1), /*cap*/ 1);

  // Assert
  EXPECT_TRUE(does_respect);
}

TEST_F(BraveAdsTimeConstraintUtilTest, DoesNotRespect) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());

  // Act
  const bool does_respect = DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint*/ base::Days(1), /*cap*/ 1);

  // Assert
  EXPECT_FALSE(does_respect);
}

}  // namespace brave_ads
