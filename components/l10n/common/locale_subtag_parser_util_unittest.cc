/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/locale_subtag_parser_util.h"

#include "brave/components/l10n/common/locale_subtag_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=LocaleParserUtilTest*

namespace brave_l10n {

TEST(LocaleParserUtilTest, Empty) {
  const LocaleSubtagInfo expected_locale_subtag;
  EXPECT_EQ(expected_locale_subtag, ParseLocaleSubtags({}));
}

TEST(LocaleParserUtilTest, Language) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  EXPECT_EQ(expected_locale_subtag, ParseLocaleSubtags("LANGUAGE"));
}

TEST(LocaleParserUtilTest, LanguageCodeSet) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  expected_locale_subtag.charset = "UTF-8";
  EXPECT_EQ(expected_locale_subtag, ParseLocaleSubtags("LANGUAGE.UTF-8"));
}

TEST(LocaleParserUtilTest, LanguageCodeSetVariant) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  expected_locale_subtag.charset = "UTF-8";
  expected_locale_subtag.variant = "variant";
  EXPECT_EQ(expected_locale_subtag,
            ParseLocaleSubtags("LANGUAGE.UTF-8@variant"));
}

TEST(LocaleParserUtilTest, LanguageCountry) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  expected_locale_subtag.country = "COUNTRY";
  EXPECT_EQ(expected_locale_subtag, ParseLocaleSubtags("LANGUAGE_country"));
}

TEST(LocaleParserUtilTest, LanguageCountryCodeSet) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  expected_locale_subtag.country = "COUNTRY";
  expected_locale_subtag.charset = "UTF-8";
  EXPECT_EQ(expected_locale_subtag,
            ParseLocaleSubtags("LANGUAGE_country.UTF-8"));
}

TEST(LocaleParserUtilTest, LanguageCountryCodeSetVariant) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  expected_locale_subtag.country = "COUNTRY";
  expected_locale_subtag.charset = "UTF-8";
  expected_locale_subtag.variant = "variant";
  EXPECT_EQ(expected_locale_subtag,
            ParseLocaleSubtags("LANGUAGE_country.UTF-8@variant"));
}

TEST(LocaleParserUtilTest, LanguageScriptCountry) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  expected_locale_subtag.script = "Script";
  expected_locale_subtag.country = "COUNTRY";
  EXPECT_EQ(expected_locale_subtag,
            ParseLocaleSubtags("LANGUAGE_sCRIPT_country"));
}

TEST(LocaleParserUtilTest, LanguageScriptCountryCodeSet) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  expected_locale_subtag.script = "Script";
  expected_locale_subtag.country = "COUNTRY";
  expected_locale_subtag.charset = "UTF-8";
  EXPECT_EQ(expected_locale_subtag,
            ParseLocaleSubtags("LANGUAGE_sCRIPT_country.UTF-8"));
}

TEST(LocaleParserUtilTest, LanguageScriptCountryCodeSetVariant) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  expected_locale_subtag.script = "Script";
  expected_locale_subtag.country = "COUNTRY";
  expected_locale_subtag.charset = "UTF-8";
  expected_locale_subtag.variant = "variant";
  EXPECT_EQ(expected_locale_subtag,
            ParseLocaleSubtags("LANGUAGE_sCRIPT_country.UTF-8@variant"));
}

TEST(LocaleParserUtilTest, Normalize) {
  LocaleSubtagInfo expected_locale_subtag;
  expected_locale_subtag.language = "language";
  expected_locale_subtag.script = "Script";
  expected_locale_subtag.country = "COUNTRY";
  expected_locale_subtag.charset = "UTF-8";
  expected_locale_subtag.variant = "variant";
  EXPECT_EQ(expected_locale_subtag,
            ParseLocaleSubtags("LANGUAGE-sCRIPT-country.UTF-8@variant"));
}

}  // namespace brave_l10n
