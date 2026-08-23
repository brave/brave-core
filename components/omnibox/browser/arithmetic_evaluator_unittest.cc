/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/arithmetic_evaluator.h"

#include "base/i18n/number_formatting.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace omnibox {

namespace {

// Expectations are built with the same formatters the evaluator uses, so the
// tests assert the arithmetic rather than the active locale's separators.
std::optional<std::u16string> Integer(int64_t value) {
  return base::FormatNumber(value);
}

std::optional<std::u16string> Decimal(double value, int fractional_digits) {
  return base::FormatDouble(value, 0, fractional_digits);
}

}  // namespace

TEST(ArithmeticEvaluatorTest, EvaluatesExactly) {
  // The reported bug: the trailing digit pushed this past the suspicious-query
  // digit limit, so it was answered from a stale prefix as "9500+780 = 10280".
  EXPECT_EQ(Integer(17304), EvaluateArithmeticExpression(u"9500+7804"));
  EXPECT_EQ(Integer(10280), EvaluateArithmeticExpression(u"9500+780"));

  // Operands longer than the digit heuristic allows.
  EXPECT_EQ(Integer(246913578),
            EvaluateArithmeticExpression(u"123456789 + 123456789"));
  EXPECT_EQ(Integer(1000000000001),
            EvaluateArithmeticExpression(u"1000000000000 + 1"));

  EXPECT_EQ(Integer(4), EvaluateArithmeticExpression(u"2+2"));
  EXPECT_EQ(Integer(17304), EvaluateArithmeticExpression(u"  9500 + 7804  "));
}

TEST(ArithmeticEvaluatorTest, RespectsPrecedenceAndAssociativity) {
  EXPECT_EQ(Integer(14), EvaluateArithmeticExpression(u"2 + 3 * 4"));
  EXPECT_EQ(Integer(10), EvaluateArithmeticExpression(u"2 * 3 + 4"));
  EXPECT_EQ(Integer(20), EvaluateArithmeticExpression(u"(2+3)*4"));
  EXPECT_EQ(Integer(1024), EvaluateArithmeticExpression(u"2^10"));

  // '^' binds tighter than unary minus.
  EXPECT_EQ(Integer(-4), EvaluateArithmeticExpression(u"-2^2"));
}

TEST(ArithmeticEvaluatorTest, EvaluatesTerminatingFractions) {
  EXPECT_EQ(Decimal(2.5, 1), EvaluateArithmeticExpression(u"10 / 4"));
  EXPECT_EQ(Decimal(3.75, 2), EvaluateArithmeticExpression(u"1.5 + 2.25"));
  EXPECT_EQ(Decimal(0.25, 2), EvaluateArithmeticExpression(u"2^-2"));
}

// Anything we can't answer exactly falls through to the caller's normal
// behavior, which for the omnibox means asking the suggest server as before.
TEST(ArithmeticEvaluatorTest, DeclinesWhatItCannotAnswerExactly) {
  // Non-terminating decimal.
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"10 / 3"));
  // Division by zero.
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"1 / 0"));
  // int64 overflow.
  EXPECT_EQ(std::nullopt,
            EvaluateArithmeticExpression(u"9223372036854775807 + 1"));
  // Exponents that are unbounded or would give an irrational result.
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"2^1000"));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"2^0.5"));
}

TEST(ArithmeticEvaluatorTest, DeclinesMalformedExpressions) {
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"1 + "));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"2 + apples"));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"(1 + 2"));
  // Grouping separators are ambiguous across locales, so they aren't parsed.
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"1,5 + 2"));
}

// '-' and '/' are used to delimit card numbers, phone numbers, SSN, and dates.
// We don't want those to evaluated as arithmetic because the user shouldn't be
// expecting this and the result wouldn't have any value. Spaces should be used
// between the operators to be explicit.
TEST(ArithmeticEvaluatorTest, DeclinesFormattedIdentifiers) {
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"1234-5678-9012-3456"));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"555-123-4567"));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"123-45-6789"));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"24/7"));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"9/11"));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"12/25/2024"));

  // Input with spaces, the same operators are unambiguous.
  EXPECT_EQ(Integer(250000), EvaluateArithmeticExpression(u"1000000 / 4"));
  EXPECT_EQ(Integer(-4444), EvaluateArithmeticExpression(u"1234 - 5678"));
}

TEST(ArithmeticEvaluatorTest, DeclinesNonArithmetic) {
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u""));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"9500"));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"hello world"));
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"+"));
  // Parentheses alone aren't an operator, so this stays a plain query.
  EXPECT_EQ(std::nullopt, EvaluateArithmeticExpression(u"(1234567890)"));
}

}  // namespace omnibox
