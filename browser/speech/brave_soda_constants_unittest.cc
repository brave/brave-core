/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <set>
#include <string_view>
#include <utility>

#include "base/i18n/language_tag.h"
#include "base/i18n/tag_converters.h"
#include "components/soda/constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speech {

namespace {

std::optional<std::string_view> Resolve(std::string_view language) {
  const BraveOnDeviceSpeechLocale* locale =
      ResolveBraveOnDeviceSpeechLocale(language);
  return locale ? std::optional(locale->locale) : std::nullopt;
}

}  // namespace

// `install()` records a locale by its name and settles it by its code.
TEST(BraveSodaConstantsUnitTest, LocalesAreUniqueAndCanonical) {
  std::set<std::string_view> names;
  std::set<LanguageCode> codes;
  for (const BraveOnDeviceSpeechLocale& locale :
       GetBraveOnDeviceSpeechLocales()) {
    SCOPED_TRACE(locale.locale);
    EXPECT_TRUE(names.insert(locale.locale).second);
    EXPECT_TRUE(codes.insert(locale.language_code).second);
    std::optional<base::i18n::LanguageTag> tag =
        base::i18n::GetLanguageTagFromString(locale.locale);
    ASSERT_TRUE(tag);
    EXPECT_EQ(locale.locale, tag->tag_string());
    EXPECT_EQ(locale.locale, Resolve(locale.locale));
  }
}

TEST(BraveSodaConstantsUnitTest, ResolvesPageTags) {
  // "fr" is as close to fr-CA as to fr-FR. The first in the table wins.
  const std::pair<std::string_view, std::string_view> kCases[] = {
      {"EN-gb", "en-US"},  // Every English tag.
      {"de_AT", "de-DE"}, {"fr", "fr-FR"}, {"es", "es-ES"}, {"es-MX", "es-US"},
      {"pt", "pt-BR"},    {"ar-AR", "ar"},  // The model card's label.
      {"zh", "zh-CN"},    {"no", "nb-NO"},
  };
  for (const auto& [language, locale] : kCases) {
    EXPECT_EQ(locale, Resolve(language)) << language;
  }
}

TEST(BraveSodaConstantsUnitTest, RefusesTagsThatNoModelServes) {
  // Traditional Chinese, Nynorsk (refused, else it matches nb-NO), a language
  // that Brave's list leaves out, and tags that do not parse.
  for (std::string_view language : {"zh-TW", "nn", "th-TH", "", "not a tag"}) {
    EXPECT_EQ(std::nullopt, Resolve(language)) << language;
  }
}

// The SODA lookup that `available()` and `install()` use.
TEST(BraveSodaConstantsUnitTest, SodaLookupGivesTheLocale) {
  std::optional<SodaLanguagePackComponentConfig> config =
      GetLanguageComponentConfigMatchingLanguageSubtag("de-AT");
  ASSERT_TRUE(config);
  EXPECT_EQ("de-DE", config->language_name);
  EXPECT_EQ(LanguageCode::kDeDe, config->language_code);
  EXPECT_FALSE(GetLanguageComponentConfigMatchingLanguageSubtag("zh-TW"));
}

TEST(BraveSodaConstantsUnitTest, NamesAndCodesRoundTrip) {
  for (const BraveOnDeviceSpeechLocale& locale :
       GetBraveOnDeviceSpeechLocales()) {
    SCOPED_TRACE(locale.locale);
    EXPECT_EQ(locale.language_code, GetLanguageCode(locale.locale));
    EXPECT_EQ(locale.locale, GetLanguageName(locale.language_code));
  }
  // Other names still use SODA's table.
  EXPECT_EQ(LanguageCode::kThTh, GetLanguageCode("th-TH"));
}

}  // namespace speech
