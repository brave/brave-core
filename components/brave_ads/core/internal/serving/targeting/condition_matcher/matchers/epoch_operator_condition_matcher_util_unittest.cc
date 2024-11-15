/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/epoch_operator_condition_matcher_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEpochOperatorConditionMatcherUtilTest : public test::TestBase {};

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest, IsOperator) {
  // Act & Assert
  EXPECT_TRUE(IsEpochOperator("[T=]:1"));
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest, IsNotOperator) {
  // Act & Assert
  EXPECT_FALSE(IsEpochOperator("[R=]:1"));
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest, DoNotMatchNonOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchEpochOperator(
      "13372214400000000" /*1st October 2024 00:00:00 UTC*/, "baz"));
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       DoNotMatchMalformedOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchEpochOperator(
      "13372214400000000" /*1st October 2024 00:00:00 UTC*/, "[T=]: 7 "));
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest, MatchEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T=]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest, DoNotMatchEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T=]:3"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest, MatchNotEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T≠]:3"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       DoNotMatchNotEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T≠]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       MatchGreaterThanOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T>]:1"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       DoNotMatchGreaterThanOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T>]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       MatchGreaterThanOrEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T≥]:1"));  // Event occurred 2 days ago.
  EXPECT_TRUE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T≥]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       DoNotMatchGreaterThanOrEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T≥]:3"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest, MatchLessThanOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T<]:3"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       DoNotMatchLessThanOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T<]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       MatchLessThanOrEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T≤]:3"));  // Event occurred 2 days ago.
  EXPECT_TRUE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T≤]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       DoNotMatchLessThanOrEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchEpochOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                         "[T≤]:1"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsEpochOperatorConditionMatcherUtilTest,
       DoNotMatchUnknownOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(MatchEpochOperator(
      "13372214400000000" /*1st October 2024 00:00:00 UTC*/, "[T_]:2"));
}

}  // namespace brave_ads
