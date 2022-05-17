/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/time_constraint_util.h"

#include <vector>

#include "base/time/time.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTimeConstraintUtilTest : public UnitTestBase {
 protected:
  BatAdsTimeConstraintUtilTest() = default;

  ~BatAdsTimeConstraintUtilTest() override = default;
};

TEST_F(BatAdsTimeConstraintUtilTest, DoesRespectWhenNoHistoory) {
  // Arrange
  std::vector<base::Time> history;

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
  FastForwardClockBy(base::Days(1));

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
