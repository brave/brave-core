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
  // Act & Assert
  EXPECT_TRUE(DoesHistoryRespectRollingTimeConstraint(
      /*history=*/{}, /*time_constraint=*/base::Days(1), /*cap=*/1));
}

TEST_F(BraveAdsTimeConstraintUtilTest, DoNotRespectTimeConstraintIfCapIsZero) {
  // Act & Assert
  EXPECT_FALSE(DoesHistoryRespectRollingTimeConstraint(
      /*history=*/{}, /*time_constraint=*/base::Days(1), /*cap=*/0));
}

TEST_F(BraveAdsTimeConstraintUtilTest,
       DoesRespectTimeConstraintIfNotExceededCap) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());

  // Act & Assert
  EXPECT_TRUE(DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint=*/base::Days(1), /*cap=*/2));
}

TEST_F(BraveAdsTimeConstraintUtilTest,
       DoesNotRespectTimeConstraintIfExceededCap) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());
  history.push_back(Now());

  // Act & Assert
  EXPECT_FALSE(DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint=*/base::Days(1), /*cap=*/2));
}

TEST_F(BraveAdsTimeConstraintUtilTest,
       DoesRespectTimeConstraintIfTimeConstraintHasPassed) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint=*/base::Days(1), /*cap=*/1));
}

TEST_F(BraveAdsTimeConstraintUtilTest,
       DoesRespectTimeConstraintOnCuspIfNotExceededCap) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_TRUE(DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint=*/base::Days(1), /*cap=*/2));
}

TEST_F(BraveAdsTimeConstraintUtilTest,
       DoesNotRespectTimeConstraintOnCuspIfExceededCap) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint=*/base::Days(1), /*cap=*/1));
}

TEST_F(
    BraveAdsTimeConstraintUtilTest,
    DoesRespectTimeConstraintIfHistorySizeIsHigherThanRemainingCapAndTimeConstraintHasPassed) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());
  history.push_back(Now());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint=*/base::Days(1), /*cap=*/1));
}

TEST_F(BraveAdsTimeConstraintUtilTest,
       DoesNotRespectTimeConstraintIfHistorySizeIsHigherThanRemainingCap) {
  // Arrange
  std::vector<base::Time> history;
  history.push_back(Now());
  history.push_back(Now());

  // Act & Assert
  EXPECT_FALSE(DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint=*/base::Days(1), /*cap=*/1));
}

}  // namespace brave_ads
