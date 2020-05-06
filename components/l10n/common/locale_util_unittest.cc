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
    RegionCodeForEnglish) {
  // Arrange
  const std::string locale = "en";

  // Act
  const std::string region_code = brave_l10n::GetRegionCode(locale);

  // Assert
  const std::string expected_region_code = "US";
  EXPECT_EQ(expected_region_code, region_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeDashRegionCode) {
  // Arrange
  const std::string locale = "en-US";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    RegionCodeForLanguageCodeDashRegionCode) {
  // Arrange
  const std::string locale = "en-US";

  // Act
  const std::string region_code = brave_l10n::GetRegionCode(locale);

  // Assert
  const std::string expected_region_code = "US";
  EXPECT_EQ(expected_region_code, region_code);
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
    RegionCodeForLanguageCodeDashWorld) {
  // Arrange
  const std::string locale = "en-101";

  // Act
  const std::string region_code = brave_l10n::GetRegionCode(locale);

  // Assert
  const std::string expected_region_code = "101";
  EXPECT_EQ(expected_region_code, region_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeDashRegionCodeDotEncoding) {
  // Arrange
  const std::string locale = "en-US.UTF-8";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    RegionCodeForLanguageCodeDashRegionCodeDotEncoding) {
  // Arrange
  const std::string locale = "en-US.UTF-8";

  // Act
  const std::string region_code = brave_l10n::GetRegionCode(locale);

  // Assert
  const std::string expected_region_code = "US";
  EXPECT_EQ(expected_region_code, region_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeDashScriptDashRegionCode) {
  // Arrange
  const std::string locale = "az-Latn-AZ";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "az";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    RegionCodeForLanguageCodeDashScriptDashRegionCode) {
  // Arrange
  const std::string locale = "az-Latn-AZ";

  // Act
  const std::string region_code = brave_l10n::GetRegionCode(locale);

  // Assert
  const std::string expected_region_code = "AZ";
  EXPECT_EQ(expected_region_code, region_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeUnderscoreRegionCode) {
  // Arrange
  const std::string locale = "en_US";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    RegionCodeForLanguageCodeUnderscoreRegionCode) {
  // Arrange
  const std::string locale = "en_US";

  // Act
  const std::string region_code = brave_l10n::GetRegionCode(locale);

  // Assert
  const std::string expected_region_code = "US";
  EXPECT_EQ(expected_region_code, region_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeUnderscoreRegionCodeDotEncoding) {
  // Arrange
  const std::string locale = "en_US.UTF-8";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "en";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    RegionCodeForLanguageCodeUnderscoreRegionCodeDotEncoding) {
  // Arrange
  const std::string locale = "en_US.UTF-8";

  // Act
  const std::string region_code = brave_l10n::GetRegionCode(locale);

  // Assert
  const std::string expected_region_code = "US";
  EXPECT_EQ(expected_region_code, region_code);
}

TEST(BatAdsLocaleTest,
    LanguageCodeForLanguageCodeUnderscoreScriptUnderscoreRegionCode) {
  // Arrange
  const std::string locale = "az_Latn_AZ";

  // Act
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  // Assert
  const std::string expected_language_code = "az";
  EXPECT_EQ(expected_language_code, language_code);
}

TEST(BatAdsLocaleTest,
    RegionCodeForLanguageCodeUnderscoreScriptUnderscoreRegionCode) {
  // Arrange
  const std::string locale = "az_Latn_AZ";

  // Act
  const std::string region_code = brave_l10n::GetRegionCode(locale);

  // Assert
  const std::string expected_region_code = "AZ";
  EXPECT_EQ(expected_region_code, region_code);
}

}  // namespace ads
