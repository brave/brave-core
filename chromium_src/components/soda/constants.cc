/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/soda/constants.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

#include "base/strings/string_util.h"

namespace speech {

// The names in this namespace are file-local. The plaster changes to the
// upstream source below use them, because that source is in this file.
namespace {

// A locale of Brave's on-device speech recognition that SODA has no language
// config for.
struct BraveOnDeviceSpeechLocale {
  std::string_view tag;
  LanguageCode language_code;
};

// Each must be a locale of the catalog in
// brave/components/local_ai/core/on_device_speech_locales.cc. SODA's own table
// has the other locales of the catalog, with the same tags. It has nl-NL only
// on ChromeOS. SODA has the codes of es-US, fr-CA and nl-NL, and Brave adds the
// other codes to `LanguageCode`.
constexpr auto kLocalesWithoutSodaConfig =
    std::to_array<BraveOnDeviceSpeechLocale>({
        {"ar", LanguageCode::kAr},
        {"es-US", LanguageCode::kEsUs},
        {"fr-CA", LanguageCode::kFrCa},
        {"nl-NL", LanguageCode::kNlNl},
        {"pt-PT", LanguageCode::kPtPt},
        {"uk-UA", LanguageCode::kUkUa},
    });

// The locale whose tag is `tag`, ignoring case, or null.
const BraveOnDeviceSpeechLocale* FindBraveOnDeviceSpeechLocaleByName(
    std::string_view tag) {
  auto it = std::ranges::find_if(
      kLocalesWithoutSodaConfig,
      [tag](const BraveOnDeviceSpeechLocale& candidate) {
        return base::EqualsCaseInsensitiveASCII(candidate.tag, tag);
      });
  return it != kLocalesWithoutSodaConfig.end() ? &*it : nullptr;
}

// The locale whose code is `language_code`, or null.
const BraveOnDeviceSpeechLocale* FindBraveOnDeviceSpeechLocaleByCode(
    LanguageCode language_code) {
  auto it = std::ranges::find(kLocalesWithoutSodaConfig, language_code,
                              &BraveOnDeviceSpeechLocale::language_code);
  return it != kLocalesWithoutSodaConfig.end() ? &*it : nullptr;
}

// The SODA language config of the locale whose tag is `language_name`, or none.
// Its name is the locale's tag and its code is the locale's code. Only SODA's
// own install code reads the other fields, and Brave does not run it.
std::optional<SodaLanguagePackComponentConfig>
GetBraveOnDeviceSpeechLanguageConfig(std::string_view language_name) {
  const BraveOnDeviceSpeechLocale* locale =
      FindBraveOnDeviceSpeechLocaleByName(language_name);
  if (!locale) {
    return std::nullopt;
  }
  return SodaLanguagePackComponentConfig{
      .language_code = locale->language_code,
      .language_name = locale->tag,
  };
}

}  // namespace

}  // namespace speech

#include <components/soda/constants.cc>
