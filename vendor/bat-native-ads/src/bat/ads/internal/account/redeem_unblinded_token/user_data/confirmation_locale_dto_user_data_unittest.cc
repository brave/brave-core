/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/redeem_unblinded_token/user_data/confirmation_locale_dto_user_data.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsConfirmationLocaleDtoUserDataTest : public UnitTestBase {
 protected:
  BatAdsConfirmationLocaleDtoUserDataTest() = default;

  ~BatAdsConfirmationLocaleDtoUserDataTest() override = default;
};

TEST_F(BatAdsConfirmationLocaleDtoUserDataTest,
       GetLocaleForNonReleaseBuildChannel) {
  // Arrange
  SetBuildChannel(false, "beta");
  MockLocaleHelper(locale_helper_mock_, "en-GB");

  // Act
  base::DictionaryValue locale = dto::user_data::GetLocale();

  // Assert
  base::DictionaryValue expected_locale;

  EXPECT_EQ(expected_locale, locale);
}

TEST_F(BatAdsConfirmationLocaleDtoUserDataTest,
       GetLocaleForReleaseBuildChannel) {
  // Arrange
  SetBuildChannel(true, "release");
  MockLocaleHelper(locale_helper_mock_, "en-GB");

  // Act
  base::DictionaryValue locale = dto::user_data::GetLocale();

  // Assert
  base::DictionaryValue expected_locale;
  expected_locale.SetKey("countryCode", base::Value("GB"));

  EXPECT_EQ(expected_locale, locale);
}

TEST_F(BatAdsConfirmationLocaleDtoUserDataTest,
       GetLocaleForCountryNotInAnonymitySet) {
  // Arrange
  SetBuildChannel(true, "release");
  MockLocaleHelper(locale_helper_mock_, "en-MC");

  // Act
  base::DictionaryValue locale = dto::user_data::GetLocale();

  // Assert
  base::DictionaryValue expected_locale;

  EXPECT_EQ(expected_locale, locale);
}

TEST_F(BatAdsConfirmationLocaleDtoUserDataTest,
       GetLocaleForCountryNotInAnonymitySetButShouldClassifyAsOther) {
  // Arrange
  SetBuildChannel(true, "release");
  MockLocaleHelper(locale_helper_mock_, "en-CX");

  // Act
  base::DictionaryValue locale = dto::user_data::GetLocale();

  // Assert
  base::DictionaryValue expected_locale;
  expected_locale.SetKey("countryCode", base::Value("??"));

  EXPECT_EQ(expected_locale, locale);
}

}  // namespace ads
