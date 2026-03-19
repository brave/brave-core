/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/numerical_operator_condition_matcher_util.h"

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNumericalOperatorConditionMatcherUtilTest
    : public test::TestBase {};

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       GetNumericalOperatorType) {
  // Act & Assert
  EXPECT_EQ(ConditionMatcherNumericalOperatorType::kEqual,
            MaybeGetNumericalOperatorType("[R=]:1"));
  EXPECT_EQ(ConditionMatcherNumericalOperatorType::kNotEqual,
            MaybeGetNumericalOperatorType("[R≠]:1"));
  EXPECT_EQ(ConditionMatcherNumericalOperatorType::kGreaterThan,
            MaybeGetNumericalOperatorType("[R>]:1"));
  EXPECT_EQ(ConditionMatcherNumericalOperatorType::kGreaterThanOrEqual,
            MaybeGetNumericalOperatorType("[R≥]:1"));
  EXPECT_EQ(ConditionMatcherNumericalOperatorType::kLessThan,
            MaybeGetNumericalOperatorType("[R<]:1"));
  EXPECT_EQ(ConditionMatcherNumericalOperatorType::kLessThanOrEqual,
            MaybeGetNumericalOperatorType("[R≤]:1"));
  EXPECT_EQ(ConditionMatcherNumericalOperatorType::kGreaterThan,
            MaybeGetNumericalOperatorType("[R>]:some.pref.path"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotGetNumericalOperatorTypeForNonOperator) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetNumericalOperatorType("[T=]:1"));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotGetNumericalOperatorTypeForEmptyCondition) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetNumericalOperatorType(""));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       ResolveNumericalOperandForLiteralInteger) {
  // Act
  const auto numerical_operand =
      MaybeResolveNumericalOperand("[R=]:42", /*virtual_prefs=*/{});
  ASSERT_TRUE(numerical_operand);

  // Assert
  EXPECT_DOUBLE_EQ(42.0, *numerical_operand);
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       ResolveNumericalOperandForLiteralFloat) {
  // Act
  const auto numerical_operand =
      MaybeResolveNumericalOperand("[R=]:3.14", /*virtual_prefs=*/{});
  ASSERT_TRUE(numerical_operand);

  // Assert
  EXPECT_DOUBLE_EQ(3.14, *numerical_operand);
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       ResolveNumericalOperandForPrefPath) {
  // Act
  const auto numerical_operand = MaybeResolveNumericalOperand(
      "[R>]:[virtual]:foo",
      /*virtual_prefs=*/base::DictValue().Set("[virtual]:foo", 7.0));
  ASSERT_TRUE(numerical_operand);

  // Assert
  EXPECT_DOUBLE_EQ(7.0, *numerical_operand);
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotResolveNumericalOperandForEmptyCondition) {
  // Act & Assert
  EXPECT_FALSE(MaybeResolveNumericalOperand("", /*virtual_prefs=*/{}));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotResolveNumericalOperandForMissingColonInCondition) {
  // Act & Assert
  EXPECT_FALSE(MaybeResolveNumericalOperand("[R=]", /*virtual_prefs=*/{}));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotResolveNumericalOperandForEmptyOperandInCondition) {
  // Act & Assert
  EXPECT_FALSE(MaybeResolveNumericalOperand("[R=]:", /*virtual_prefs=*/{}));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotResolveNumericalOperandForMalformedInput) {
  // Act & Assert
  EXPECT_FALSE(MaybeResolveNumericalOperand("[R=]: 1 ", /*virtual_prefs=*/{}));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotResolveNumericalOperandForUnknownPrefPath) {
  // Act & Assert
  EXPECT_FALSE(MaybeResolveNumericalOperand("[R=]:[virtual]:missing",
                                            /*virtual_prefs=*/{}));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotResolveNumericalOperandForNonNumericPrefPath) {
  // Act & Assert
  EXPECT_FALSE(
      MaybeResolveNumericalOperand("[R=]:[virtual]:foo",
                                   /*virtual_prefs=*/base::DictValue().Set(
                                       "[virtual]:foo", "not_a_number")));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchNumericalOperatorForEmptyValue) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator(
      "", ConditionMatcherNumericalOperatorType::kEqual, 0));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest, MatchEqualOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kEqual, 1));
  EXPECT_TRUE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kEqual, 1));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchEqualOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kEqual, 2));
  EXPECT_FALSE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kEqual, 2));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchNotEqualOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kNotEqual, 2));
  EXPECT_TRUE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kNotEqual, 2));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchNotEqualOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kNotEqual, 1));
  EXPECT_FALSE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kNotEqual, 1));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchGreaterThanOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kGreaterThan, 0));
  EXPECT_TRUE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kGreaterThan, 0));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchGreaterThanOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kGreaterThan, 1));
  EXPECT_FALSE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kGreaterThan, 1));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchGreaterThanOrEqualOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kGreaterThanOrEqual, 0));
  EXPECT_TRUE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kGreaterThanOrEqual, 0));

  EXPECT_TRUE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kGreaterThanOrEqual, 1));
  EXPECT_TRUE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kGreaterThanOrEqual, 1));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchGreaterThanOrEqualOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kGreaterThanOrEqual, 2));
  EXPECT_FALSE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kGreaterThanOrEqual, 2));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchLessThanOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kLessThan, 2));
  EXPECT_TRUE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kLessThan, 2));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchLessThanOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kLessThan, 1));
  EXPECT_FALSE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kLessThan, 1));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       MatchLessThanOrEqualOperator) {
  // Act & Assert
  EXPECT_TRUE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kLessThanOrEqual, 1));
  EXPECT_TRUE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kLessThanOrEqual, 1));

  EXPECT_TRUE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kLessThanOrEqual, 2));
  EXPECT_TRUE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kLessThanOrEqual, 2));
}

TEST_F(BraveAdsNumericalOperatorConditionMatcherUtilTest,
       DoNotMatchLessThanOrEqualOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchNumericalOperator(
      "1.0", ConditionMatcherNumericalOperatorType::kLessThanOrEqual, 0));
  EXPECT_FALSE(MatchNumericalOperator(
      "1", ConditionMatcherNumericalOperatorType::kLessThanOrEqual, 0));
}

}  // namespace brave_ads
