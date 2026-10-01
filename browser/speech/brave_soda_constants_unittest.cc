/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>
#include <string_view>

#include "components/soda/constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speech {

namespace {

std::optional<std::string> GetPackName(std::string_view language) {
  std::optional<SodaLanguagePackComponentConfig> config =
      GetLanguageComponentConfigMatchingLanguageSubtag(language);
  if (!config) {
    return std::nullopt;
  }
  return std::string(config->language_name);
}

}  // namespace

TEST(BraveSodaConstantsUnitTest, SupportsAnyEnglishTag) {
  for (std::string_view language : {"en-US", "en-GB", "en-AU", "en", "EN-gb"}) {
    EXPECT_TRUE(IsBraveOnDeviceSpeechLanguageSupported(language)) << language;
  }
}

TEST(BraveSodaConstantsUnitTest, SupportsExactMultilingualTagsIgnoringCase) {
  for (std::string_view language :
       {"es-ES", "es-US", "it-IT", "pt-BR", "pt-PT", "hi-IN", "ES-us"}) {
    EXPECT_TRUE(IsBraveOnDeviceSpeechLanguageSupported(language)) << language;
  }

  // The multilingual model has no prompt for these.
  for (std::string_view language : {"es-MX", "es", "it-CH", "pt-AO", "hi"}) {
    EXPECT_FALSE(IsBraveOnDeviceSpeechLanguageSupported(language)) << language;
  }
  for (std::string_view language : {"fr-FR", "de-DE", ""}) {
    EXPECT_FALSE(IsBraveOnDeviceSpeechLanguageSupported(language)) << language;
  }
}

// `available()` and `install()` refuse a language with no pack before asking
// Brave, so a pack must be found exactly for the languages Brave supports.
TEST(BraveSodaConstantsUnitTest, OnlySupportedLanguagesResolveToAPack) {
  EXPECT_EQ("en-US", GetPackName("en-AU"));
  EXPECT_EQ("es-ES", GetPackName("es-US"));
  EXPECT_EQ("pt-BR", GetPackName("pt-PT"));
  EXPECT_EQ("hi-IN", GetPackName("HI-in"));

  // SODA has a pack for each of these, and Brave refuses them anyway.
  EXPECT_EQ(std::nullopt, GetPackName("es-MX"));
  EXPECT_EQ(std::nullopt, GetPackName("fr-FR"));
}

// The pack names are what Brave is then asked about, so each must be
// supported itself and map to a language code `install()` can track.
TEST(BraveSodaConstantsUnitTest, PackNamesAreSupportedAndTrackable) {
  EXPECT_THAT(
      GetBraveOnDeviceSpeechSodaLanguageNames(),
      testing::ElementsAre("en-US", "es-ES", "it-IT", "pt-BR", "hi-IN"));
  for (const std::string& name : GetBraveOnDeviceSpeechSodaLanguageNames()) {
    EXPECT_TRUE(IsBraveOnDeviceSpeechLanguageSupported(name)) << name;
    EXPECT_NE(LanguageCode::kNone, GetLanguageCode(name)) << name;
  }
}

}  // namespace speech
