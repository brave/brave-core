/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_locales.h"

#include <optional>
#include <string_view>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace local_ai {

namespace {

std::optional<std::string_view> Resolve(std::string_view language) {
  const OnDeviceSpeechLocale* locale = ResolveOnDeviceSpeechLocale(language);
  return locale ? std::optional(locale->tag) : std::nullopt;
}

}  // namespace

// Each tag has one row, and each row resolves to itself.
TEST(OnDeviceSpeechLocalesUnitTest, EachLocaleResolvesToItself) {
  absl::flat_hash_set<std::string_view> tags;
  for (const OnDeviceSpeechLocale& locale : GetOnDeviceSpeechLocales()) {
    EXPECT_TRUE(tags.insert(locale.tag).second) << locale.tag;
    EXPECT_EQ(locale.tag, Resolve(locale.tag));
  }
}

// en-US is the one English locale. It serves the language alone and each
// other region.
TEST(OnDeviceSpeechLocalesUnitTest, ResolvesEachEnglishTagToEnUS) {
  EXPECT_EQ("en-US", Resolve("en"));
  EXPECT_EQ("en-US", Resolve("en-GB"));
}

// A region can select a locale that is not the first of its language. See
// `GetLocaleMatcher()`.
TEST(OnDeviceSpeechLocalesUnitTest, ResolvesSpanishAndPortugueseByRegion) {
  EXPECT_EQ("es-ES", Resolve("es"));
  EXPECT_EQ("es-US", Resolve("es-MX"));
  EXPECT_EQ("pt-BR", Resolve("pt"));
  EXPECT_EQ("pt-PT", Resolve("pt-AO"));
}

// fr-FR and fr-CA are at the same distance from "fr", so the language alone
// and the other regions get the first of the two rows, fr-FR.
TEST(OnDeviceSpeechLocalesUnitTest, ResolvesFrenchToFrFR) {
  EXPECT_EQ("fr-FR", Resolve("fr"));
  EXPECT_EQ("fr-FR", Resolve("fr-BE"));
}

TEST(OnDeviceSpeechLocalesUnitTest, RefusesTagsThatNoModelServes) {
  // Languages that no locale has, and tags that do not parse.
  for (std::string_view language :
       {"zh-CN", "zh-TW", "pl-PL", "nb-NO", "th-TH", "", "not a tag"}) {
    EXPECT_EQ(std::nullopt, Resolve(language)) << language;
  }
}

}  // namespace local_ai
