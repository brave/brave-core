/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"

#include "brave/components/brave_ads/core/internal/common/locale/test/fake_locale.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsLocaleUtilTest, CurrentLanguageCode) {
  // Arrange
  test::FakeLocale fake_locale;
  fake_locale.SetLanguageCode("en");

  // Act & Assert
  EXPECT_EQ("en", CurrentLanguageCode());
}

TEST(BraveAdsLocaleUtilTest, CurrentCountryCode) {
  // Arrange
  test::FakeLocale fake_locale;
  fake_locale.SetCountryCode("KY");

  // Act & Assert
  EXPECT_EQ("KY", CurrentCountryCode());
}

}  // namespace brave_ads
