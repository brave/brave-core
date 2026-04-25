/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/locale_util.h"

#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=LocaleUtilTest*

namespace brave_l10n {

TEST(LocaleUtilTest, ScopedDefaultLocale) {
  const test::ScopedDefaultLocale scoped_default_locale_us("en_KY");
  ASSERT_EQ("en_KY", GetDefaultLocaleString());

  {
    const test::ScopedDefaultLocale scoped_default_locale_gb("en_GB");
    ASSERT_EQ("en_GB", GetDefaultLocaleString());
  }

  EXPECT_EQ("en_KY", GetDefaultLocaleString());
}

TEST(LocaleUtilTest, GetDefaultLocaleString) {
  const test::ScopedDefaultLocale scoped_default_locale("en_Latn_US.UTF-8");
  EXPECT_EQ("en_Latn_US.UTF-8", GetDefaultLocaleString());
}

TEST(LocaleUtilTest, GetDefaultISOLanguageCodeString) {
  const test::ScopedDefaultLocale scoped_default_locale("en_Latn_US.UTF-8");
  EXPECT_EQ("en", GetDefaultISOLanguageCodeString());
}

TEST(LocaleUtilTest, GetFallbackDefaultISOLanguageCodeString) {
  const test::ScopedDefaultLocale scoped_default_locale({});
  EXPECT_EQ("en", GetDefaultISOLanguageCodeString());
}

TEST(LocaleUtilTest, GetISOLanguageCodeForLocale) {
  const test::ScopedDefaultLocale scoped_default_locale("ja_JP");
  EXPECT_EQ("en", GetISOLanguageCode("en_Latn_US.UTF-8@currency=USD"));
}

TEST(LocaleUtilTest, GetDefaultISOCountryCodeString) {
  const test::ScopedDefaultLocale scoped_default_locale("en_Latn_US.UTF-8");
  EXPECT_EQ("US", GetDefaultISOCountryCodeString());
}

TEST(LocaleUtilTest, GetFallbackDefaultISOCountryCodeString) {
  const test::ScopedDefaultLocale scoped_default_locale({});
  EXPECT_EQ("US", GetDefaultISOCountryCodeString());
}

TEST(LocaleUtilTest, GetISOCountryCodeForLocale) {
  const test::ScopedDefaultLocale scoped_default_locale("ja_JP");
  EXPECT_EQ("US", GetISOCountryCode("en_Latn_US.UTF-8@currency=USD"));
}

TEST(LocaleUtilTest, GetDefaultUNM49CodeString) {
  const test::ScopedDefaultLocale scoped_default_locale("en_001");
  EXPECT_EQ("001", GetDefaultISOCountryCodeString());
}

TEST(LocaleUtilTest, GetUNM49CodeForLocaleString) {
  const test::ScopedDefaultLocale scoped_default_locale("ja_JP");
  EXPECT_EQ("001", GetISOCountryCode("en_001"));
}

}  // namespace brave_l10n
