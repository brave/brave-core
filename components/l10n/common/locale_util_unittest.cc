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

TEST(LocaleUtilTest, GetDefaultISOScriptCodeString) {
  const test::ScopedDefaultLocale scoped_default_locale("en_Latn_US.UTF-8");
  EXPECT_EQ("Latn", GetDefaultISOScriptCodeString());
}

TEST(LocaleUtilTest, GetUnspecifiedDefaultISOScriptCodeString) {
  const test::ScopedDefaultLocale scoped_default_locale({});
  EXPECT_FALSE(GetDefaultISOScriptCodeString());
}

TEST(LocaleUtilTest, GetISOScriptCodeForLocale) {
  const test::ScopedDefaultLocale scoped_default_locale("ja_JP");
  EXPECT_EQ("Latn", GetISOScriptCode("en_Latn_US.UTF-8@currency=USD"));
}

TEST(LocaleUtilTest, GetUnspecifiedISOScriptCodeForLocale) {
  EXPECT_FALSE(GetISOScriptCode("en_US.UTF-8@currency=USD"));
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

TEST(LocaleUtilTest, GetDefaultCharSetString) {
  const test::ScopedDefaultLocale scoped_default_locale("en_Latn_US.UTF-8");
  EXPECT_EQ("UTF-8", GetDefaultCharSetString());
}

TEST(LocaleUtilTest, GetUnspecifiedDefaultCharSetString) {
  const test::ScopedDefaultLocale scoped_default_locale({});
  EXPECT_FALSE(GetDefaultCharSetString());
}

TEST(LocaleUtilTest, GetCharSetForLocale) {
  const test::ScopedDefaultLocale scoped_default_locale("ja_JP");
  EXPECT_EQ("UTF-8", GetCharSet("en_Latn_US.UTF-8@currency=USD"));
}

TEST(LocaleUtilTest, GetUnspecifiedCharSetForLocale) {
  EXPECT_FALSE(GetCharSet("en_Latn_US@currency=USD"));
}

TEST(LocaleUtilTest, GetDefaultVariantString) {
  const test::ScopedDefaultLocale scoped_default_locale(
      "en_Latn_US.UTF-8@currency=USD");
  EXPECT_EQ("currency=USD", GetDefaultVariantString());
}

TEST(LocaleUtilTest, GetUnspecifiedDefaultVariantString) {
  const test::ScopedDefaultLocale scoped_default_locale({});
  EXPECT_FALSE(GetDefaultVariantString());
}

TEST(LocaleUtilTest, GetVariantForLocale) {
  const test::ScopedDefaultLocale scoped_default_locale("ja_JP");
  EXPECT_EQ("currency=USD", GetVariant("en_Latn_US.UTF-8@currency=USD"));
}

TEST(LocaleUtilTest, GetUnspecifiedVariantForLocale) {
  EXPECT_FALSE(GetVariant("en_Latn_US.UTF-8"));
}

}  // namespace brave_l10n
