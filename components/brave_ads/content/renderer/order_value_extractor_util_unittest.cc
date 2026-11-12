/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_value_extractor_util.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/re2/src/re2/re2.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderValueExtractorUtilTest, JoinRegexesMatchesFirstPattern) {
  // Act
  const re2::RE2 regex =
      JoinRegexes(std::vector<std::string>{"(?i)EUR", "(?i)USD"});
  ASSERT_TRUE(regex.ok());

  // Assert
  EXPECT_TRUE(re2::RE2::PartialMatch("4,98 EUR", regex));
}

TEST(BraveAdsOrderValueExtractorUtilTest, JoinRegexesMatchesSecondPattern) {
  // Act
  const re2::RE2 regex =
      JoinRegexes(std::vector<std::string>{"(?i)EUR", "(?i)USD"});
  ASSERT_TRUE(regex.ok());

  // Assert
  EXPECT_TRUE(re2::RE2::PartialMatch("4.98 USD", regex));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     JoinRegexesDoesNotMatchUnlistedPattern) {
  // Act
  const re2::RE2 regex =
      JoinRegexes(std::vector<std::string>{"(?i)EUR", "(?i)USD"});
  ASSERT_TRUE(regex.ok());

  // Assert
  EXPECT_FALSE(re2::RE2::PartialMatch("4.98 GBP", regex));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     JoinRegexesOfEmptyListMatchesNothing) {
  // Act
  const re2::RE2 regex = JoinRegexes(std::vector<std::string>{});
  ASSERT_TRUE(regex.ok());

  // Assert
  EXPECT_FALSE(re2::RE2::PartialMatch("4,98 EUR", regex));
}

TEST(BraveAdsOrderValueExtractorUtilTest, ExtractBareNumberWithDecimalComma) {
  // Act & Assert
  EXPECT_EQ("4,98", ExtractBareNumber("4,98 €"));
}

TEST(BraveAdsOrderValueExtractorUtilTest, ExtractBareNumberWithDecimalPoint) {
  // Act & Assert
  EXPECT_EQ("4.98", ExtractBareNumber("$4.98"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithThousandsSeparator) {
  // Act & Assert
  EXPECT_EQ("1,234.56", ExtractBareNumber("$1,234.56"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithNonBreakingSpaceThousandsSeparator) {
  // Act & Assert
  EXPECT_EQ("1\u00A0234,56", ExtractBareNumber("1\u00A0234,56 €"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     DoNotExtractBareNumberFromNonNumericCandidate) {
  // Act & Assert
  EXPECT_FALSE(ExtractBareNumber("Not a number"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberPicksLastNumberOverEarlierReferenceNumber) {
  // Act & Assert
  EXPECT_EQ("4,98",
            ExtractBareNumber("Order placed by customer #12345, total 4,98 €"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithoutThousandsSeparator) {
  // Act & Assert
  EXPECT_EQ("42", ExtractBareNumber("42"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithDecimalPointAndNoThousandsSeparator) {
  // Act & Assert
  EXPECT_EQ("42.42", ExtractBareNumber("42.42"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberFourDigitsWithoutThousandsSeparator) {
  // Act & Assert
  EXPECT_EQ("1234", ExtractBareNumber("1234"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberFourDigitsWithDecimalAndNoThousandsSeparator) {
  // Act & Assert
  EXPECT_EQ("1234.56", ExtractBareNumber("1234.56"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberSevenDigitsWithoutThousandsSeparator) {
  // Act & Assert
  EXPECT_EQ("1000000", ExtractBareNumber("1000000"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithCommaThousandsAndDotDecimal) {
  // Act & Assert
  EXPECT_EQ("1,234.56", ExtractBareNumber("1,234.56"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithDotThousandsAndCommaDecimal) {
  // Act & Assert
  EXPECT_EQ("1.234,56", ExtractBareNumber("1.234,56"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithRegularSpaceThousandsSeparator) {
  // Act & Assert
  EXPECT_EQ("1 234,56", ExtractBareNumber("1 234,56 €"));
}

TEST(BraveAdsOrderValueExtractorUtilTest,
     ExtractBareNumberWithThinSpaceThousandsSeparator) {
  // Act & Assert
  EXPECT_EQ("1\u2009234,56", ExtractBareNumber("1\u2009234,56 €"));
}

}  // namespace brave_ads
