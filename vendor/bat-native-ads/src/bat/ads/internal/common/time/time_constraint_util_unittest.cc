/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/time/time_constraint_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTimeConstraintUtilTest : public UnitTestBase {};

TEST_F(BatAdsTimeConstraintUtilTest, DoesRespectWhenNoHistoory) {
  // Arrange
  const std::vector<base::Time> history;

  // Act
  const bool does_respect =
      DoesHistoryRespectRollingTimeConstraint(history, base::Days(1), 1);

  // Assert
  EXPECT_TRUE(does_respect);
}

TEST_F(BatAdsTimeConstraintUtilTest, DoesRespect) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());

  AdvanceClockBy(base::Days(1));

  // Act
  const bool does_respect =
      DoesHistoryRespectRollingTimeConstraint(history, base::Days(1), 1);

  // Assert
  EXPECT_TRUE(does_respect);
}

TEST_F(BatAdsTimeConstraintUtilTest, DoesNotRespect) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());

  // Act
  const bool does_respect =
      DoesHistoryRespectRollingTimeConstraint(history, base::Days(1), 1);

  // Assert
  EXPECT_FALSE(does_respect);
}

}  // namespace ads
