/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "brave/components/local_ai/core/on_device_speech_locales.h"
#include "components/soda/constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace speech {

// `install()` records each locale by its tag and settles it by its code. Each
// locale of the catalog has its own code, which gives back its tag.
TEST(BraveSodaConstantsUnitTest, EachLocaleHasItsOwnCode) {
  absl::flat_hash_set<LanguageCode> codes;
  for (const local_ai::OnDeviceSpeechLocale& locale :
       local_ai::GetOnDeviceSpeechLocales()) {
    SCOPED_TRACE(locale.tag);
    const LanguageCode code = GetLanguageCode(locale.tag);
    EXPECT_NE(LanguageCode::kNone, code);
    EXPECT_TRUE(codes.insert(code).second);
    EXPECT_EQ(locale.tag, GetLanguageName(code));
  }
}

// The SODA lookup that `available()` and `install()` use gives each locale of
// the catalog its own tag and code.
TEST(BraveSodaConstantsUnitTest, SodaLookupGivesEachLocale) {
  for (const local_ai::OnDeviceSpeechLocale& locale :
       local_ai::GetOnDeviceSpeechLocales()) {
    SCOPED_TRACE(locale.tag);
    std::optional<SodaLanguagePackComponentConfig> config =
        GetLanguageComponentConfigMatchingLanguageSubtag(locale.tag);
    ASSERT_TRUE(config);
    EXPECT_EQ(locale.tag, config->language_name);
    EXPECT_EQ(GetLanguageCode(locale.tag), config->language_code);
  }
}

// Names that are not a Brave locale without a SODA config still get SODA's own
// lookups.
TEST(BraveSodaConstantsUnitTest, OtherNamesUseSodaLookups) {
  EXPECT_EQ(LanguageCode::kThTh, GetLanguageCode("th-TH"));
  std::optional<SodaLanguagePackComponentConfig> spanish =
      GetLanguageComponentConfigMatchingLanguageSubtag("es-MX");
  ASSERT_TRUE(spanish);
  EXPECT_EQ("es-ES", spanish->language_name);
}

}  // namespace speech
