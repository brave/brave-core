/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/locale_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::user_data {

class BraveAdsLocaleUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsLocaleUserDataTest, GetLocaleForNonReleaseBuildChannel) {
  // Arrange
  MockBuildChannel(BuildChannelType::kNightly);

  // Act
  const base::Value::Dict user_data = GetLocale();

  // Assert
  const base::Value expected_user_data = base::test::ParseJson("{}");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BraveAdsLocaleUserDataTest, GetLocaleForReleaseBuildChannel) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);

  // Act
  const base::Value::Dict user_data = GetLocale();

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"countryCode":"US"})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BraveAdsLocaleUserDataTest, GetLocaleForCountryNotInAnonymitySet) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);

  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_MC"};

  // Act
  const base::Value::Dict user_data = GetLocale();

  // Assert
  const base::Value expected_user_data = base::test::ParseJson("{}");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BraveAdsLocaleUserDataTest,
       GetLocaleForCountryNotInAnonymitySetButShouldClassifyAsOther) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);

  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_CX"};

  // Act
  const base::Value::Dict user_data = GetLocale();

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"countryCode":"??"})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace brave_ads::user_data
