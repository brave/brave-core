/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/numerical_operator_condition_matcher_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNumericalOperatorConditionMatcherUtilTest
    : public test::TestBase {};

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest, IsOperator) {
  // Act & Assert
  EXPECT_TRUE(IsNumericalOperator("[R=]:1"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest, IsNotOperator) {
  // Act & Assert
  EXPECT_FALSE(IsNumericalOperator("[T=]:1"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchNonOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator("1", "baz"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchMalformedOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator("1", "[R=]: 1 "));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest, MatchEqualOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R=]:1"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R=]:1"));
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R=]:1.0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R=]:1.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchEqualOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R=]:2"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R=]:2"));
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R=]:2.0"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R=]:2.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchNotEqualOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≠]:2"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≠]:2"));
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≠]:2.0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≠]:2.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchNotEqualOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R≠]:1"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R≠]:1"));
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R≠]:1.0"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R≠]:1.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchGreaterThanOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R>]:0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R>]:0"));
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R>]:0.0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R>]:0.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchGreaterThanOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R>]:1"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R>]:1"));
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R>]:1.0"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R>]:1.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchGreaterThanOrEqualOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≥]:0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≥]:0"));
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≥]:0.0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≥]:0.0"));

  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≥]:1"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≥]:1"));
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≥]:1.0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≥]:1.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchGreaterThanOrEqualOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R≥]:2"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R≥]:2"));
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R≥]:2.0"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R≥]:2.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchLessThanOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R<]:2"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R<]:2"));
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R<]:2.0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R<]:2.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchLessThanOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R<]:1"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R<]:1"));
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R<]:1.0"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R<]:1.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchLessThanOrEqualOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≤]:1"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≤]:1"));
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≤]:1.0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≤]:1.0"));

  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≤]:2"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≤]:2"));
  EXPECT_TRUE(MatchNumericalOperator("1.0", "[R≤]:2.0"));
  EXPECT_TRUE(MatchNumericalOperator("1", "[R≤]:2.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchLessThanOrEqualOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R≤]:0"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R≤]:0"));
  EXPECT_FALSE(MatchNumericalOperator("1.0", "[R≤]:0.0"));
  EXPECT_FALSE(MatchNumericalOperator("1", "[R≤]:0.0"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchUnknownOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator("1", "[_]:2"));
}

}  // namespace brave_ads
