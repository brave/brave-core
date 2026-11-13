/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_value_extractor_util.h"

#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/re2/src/re2/re2.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderValueExtractorUtilTest, JoinRegexesMatchesFirstPattern) {
  // Act
  const std::unique_ptr<re2::RE2> regex =
      JoinRegexes(std::vector<std::string>{"(?i)EUR", "(?i)USD"});
  ASSERT_TRUE(regex);

  // Assert
  EXPECT_TRUE(re2::RE2::PartialMatch("4,98 EUR", *regex));
}

TEST(BraveAdsOrderValueExtractorUtilTest, JoinRegexesMatchesSecondPattern) {
  // Act
  const std::unique_ptr<re2::RE2> regex =
      JoinRegexes(std::vector<std::string>{"(?i)EUR", "(?i)USD"});
  ASSERT_TRUE(regex);

  // Assert
  EXPECT_TRUE(re2::RE2::PartialMatch("4.98 USD", *regex));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     JoinRegexesDoesNotMatchUnlistedPattern) {
  // Act
  const std::unique_ptr<re2::RE2> regex =
      JoinRegexes(std::vector<std::string>{"(?i)EUR", "(?i)USD"});
  ASSERT_TRUE(regex);

  // Assert
  EXPECT_FALSE(re2::RE2::PartialMatch("4.98 GBP", *regex));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     JoinRegexesOfEmptyListReturnsNullptr) {
  // Act & Assert
  EXPECT_FALSE(JoinRegexes(std::vector<std::string>{}));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     JoinRegexesDropsNonCompilingPatternButKeepsOthers) {
  // Act
  const std::unique_ptr<re2::RE2> regex = JoinRegexes(
      std::vector<std::string>{"(?i)EUR", "(unterminated", "(?i)USD"});
  ASSERT_TRUE(regex);

  // Assert
  EXPECT_TRUE(re2::RE2::PartialMatch("4,98 EUR", *regex));
  EXPECT_TRUE(re2::RE2::PartialMatch("4.98 USD", *regex));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     JoinRegexesOfAllNonCompilingPatternsReturnsNullptr) {
  // Act & Assert
  EXPECT_FALSE(
      JoinRegexes(std::vector<std::string>{"(unterminated", "[unclosed"}));
}

TEST(BraveAdsOrderValueExtractorUtilTest, MatchCurrencyReturnsMatchingCode) {
  // Arrange
  const base::flat_map<std::string, std::string> currency_regexes = {
      {"EUR", "(?i)EUR"}, {"USD", "(?i)USD"}};

  // Act & Assert
  EXPECT_EQ("USD", MaybeMatchCurrency("4.98 USD", currency_regexes));
}

TEST(BraveAdsOrderValueExtractorUtilTest, DoesNotMatchCurrencyForUnlistedCode) {
  // Arrange
  const base::flat_map<std::string, std::string> currency_regexes = {
      {"EUR", "(?i)EUR"}, {"USD", "(?i)USD"}};

  // Act & Assert
  EXPECT_FALSE(MaybeMatchCurrency("4.98 GBP", currency_regexes));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     DoesNotMatchCurrencyWithNonCompilingPattern) {
  // Arrange
  const base::flat_map<std::string, std::string> currency_regexes = {
      {"EUR", "(unterminated"}};

  // Act & Assert
  EXPECT_FALSE(MaybeMatchCurrency("4.98 EUR", currency_regexes));
}

TEST(BraveAdsOrderValueExtractorUtilTest, DoesNotMatchCurrencyWithNoRegexes) {
  // Act & Assert
  EXPECT_FALSE(MaybeMatchCurrency("4.98 EUR",
                                  base::flat_map<std::string, std::string>{}));
}

TEST(BraveAdsOrderValueExtractorUtilTest, ExtractBareNumberWithDecimalComma) {
  // Act & Assert
  EXPECT_EQ("4,98", MaybeExtractBareNumber("4,98 €"));
}

TEST(BraveAdsOrderValueExtractorUtilTest, ExtractBareNumberWithDecimalPoint) {
  // Act & Assert
  EXPECT_EQ("4.98", MaybeExtractBareNumber("$4.98"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithGroupingSeparator) {
  // Act & Assert
  EXPECT_EQ("1,234.56", MaybeExtractBareNumber("$1,234.56"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithNonBreakingSpaceGroupingSeparator) {
  // Act & Assert
  EXPECT_EQ("1 234,56", MaybeExtractBareNumber("1 234,56 €"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     DoesNotExtractBareNumberFromNonNumericCandidate) {
  // Act & Assert
  EXPECT_FALSE(MaybeExtractBareNumber("Not a number"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberPicksLastNumberOverEarlierReferenceNumber) {
  // Act & Assert
  EXPECT_EQ("4,98", MaybeExtractBareNumber(
                        "Order placed by customer #12345, total 4,98 €"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithoutGroupingSeparator) {
  // Act & Assert
  EXPECT_EQ("42", MaybeExtractBareNumber("42"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithDecimalPointAndNoGroupingSeparator) {
  // Act & Assert
  EXPECT_EQ("42.42", MaybeExtractBareNumber("42.42"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberFourDigitsWithoutGroupingSeparator) {
  // Act & Assert
  EXPECT_EQ("1234", MaybeExtractBareNumber("1234"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberFourDigitsWithDecimalAndNoGroupingSeparator) {
  // Act & Assert
  EXPECT_EQ("1234.56", MaybeExtractBareNumber("1234.56"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberSevenDigitsWithoutGroupingSeparator) {
  // Act & Assert
  EXPECT_EQ("1000000", MaybeExtractBareNumber("1000000"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithCommaGroupingAndDotDecimal) {
  // Act & Assert
  EXPECT_EQ("1,234.56", MaybeExtractBareNumber("1,234.56"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithDotGroupingAndCommaDecimal) {
  // Act & Assert
  EXPECT_EQ("1.234,56", MaybeExtractBareNumber("1.234,56"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithRegularSpaceGroupingSeparator) {
  // Act & Assert
  EXPECT_EQ("1 234,56", MaybeExtractBareNumber("1 234,56 €"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithThinSpaceGroupingSeparator) {
  // Act & Assert
  EXPECT_EQ("1 234,56", MaybeExtractBareNumber("1 234,56 €"));
}

}  // namespace brave_ads
