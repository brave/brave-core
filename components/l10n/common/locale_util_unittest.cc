/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/locale_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsLocaleTest,
    LanguageCodeForEnglish) {
  // Arrange
  const std::string locale = "en";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    CountryCodeForEnglish) {
  // Arrange
  const std::string locale = "en";

  // Act
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  // Assert
  const std::string expected_country_code = "US";
  EXPECT_EQ(expected_country_code, country_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeDashCountryCode) {
  // Arrange
  const std::string locale = "en-US";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    CountryCodeForLanguageCodeDashCountryCode) {
  // Arrange
  const std::string locale = "en-US";

  // Act
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  // Assert
  const std::string expected_country_code = "US";
  EXPECT_EQ(expected_country_code, country_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeDashWorld) {
  // Arrange
  const std::string locale = "en-101";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    CountryCodeForLanguageCodeDashWorld) {
  // Arrange
  const std::string locale = "en-101";

  // Act
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  // Assert
  const std::string expected_country_code = "101";
  EXPECT_EQ(expected_country_code, country_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeDashCountryCodeCodeDotEncoding) {
  // Arrange
  const std::string locale = "en-US.UTF-8";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    CountryCodeCodeForLanguageCodeDashCountryCodeCodeDotEncoding) {
  // Arrange
  const std::string locale = "en-US.UTF-8";

  // Act
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  // Assert
  const std::string expected_country_code = "US";
  EXPECT_EQ(expected_country_code, country_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeDashScriptDashCountryCodeCode) {
  // Arrange
  const std::string locale = "az-Latn-AZ";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "az";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    CountryCodeCodeForLanguageCodeDashScriptDashCountryCodeCode) {
  // Arrange
  const std::string locale = "az-Latn-AZ";

  // Act
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  // Assert
  const std::string expected_country_code = "AZ";
  EXPECT_EQ(expected_country_code, country_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeUnderscoreCountryCodeCode) {
  // Arrange
  const std::string locale = "en_US";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    CountryCodeCodeForLanguageCodeUnderscoreCountryCodeCode) {
  // Arrange
  const std::string locale = "en_US";

  // Act
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  // Assert
  const std::string expected_country_code = "US";
  EXPECT_EQ(expected_country_code, country_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeUnderscoreCountryCodeCodeDotEncoding) {
  // Arrange
  const std::string locale = "en_US.UTF-8";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    CountryCodeCodeForLanguageCodeUnderscoreCountryCodeCodeDotEncoding) {
  // Arrange
  const std::string locale = "en_US.UTF-8";

  // Act
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  // Assert
  const std::string expected_country_code = "US";
  EXPECT_EQ(expected_country_code, country_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeUnderscoreScriptUnderscoreCountryCodeCode) {
  // Arrange
  const std::string locale = "az_Latn_AZ";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "az";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    CountryCodeCodeForLanguageCodeUnderscoreScriptUnderscoreCountryCodeCode) {
  // Arrange
  const std::string locale = "az_Latn_AZ";

  // Act
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  // Assert
  const std::string expected_country_code = "AZ";
  EXPECT_EQ(expected_country_code, country_code);
}

}  // namespace ads
