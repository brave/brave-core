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

namespace {

// A locale that Brave's on-device speech recognition serves and SODA has no
// language config for. Only the locales of `kBraveOnlyLocales` are Brave-only.
struct BraveOnlyLocale {
  std::string_view language_name;
  LanguageCode language_code;
};

// The locales of the catalog in
// brave/components/local_ai/core/on_device_speech_locales.cc that SODA has no
// language config for on desktop. SODA's own table has the other locales of
// the catalog, with the same names. It has nl-NL only on ChromeOS. SODA has the
// codes of es-US, fr-CA and nl-NL, and Brave adds the other codes to
// `LanguageCode`.
//
// The lookups below find only these locales. For all other names and codes,
// they give null, and SODA's own lookup answers. This includes the other
// locales of the catalog, names outside the catalog and `kNone`.
constexpr auto kBraveOnlyLocales = std::to_array<BraveOnlyLocale>({
    {"ar", LanguageCode::kAr},
    {"es-US", LanguageCode::kEsUs},
    {"fr-CA", LanguageCode::kFrCa},
    {"nl-NL", LanguageCode::kNlNl},
    {"pt-PT", LanguageCode::kPtPt},
    {"uk-UA", LanguageCode::kUkUa},
});

// The Brave-only locale whose name is `language_name`, ignoring case, or null.
const BraveOnlyLocale* FindBraveOnlyLocaleByName(
    std::string_view language_name) {
  auto it = std::ranges::find_if(
      kBraveOnlyLocales, [language_name](const BraveOnlyLocale& candidate) {
        return base::EqualsCaseInsensitiveASCII(candidate.language_name,
                                                language_name);
      });
  return it != kBraveOnlyLocales.end() ? &*it : nullptr;
}

// The Brave-only locale whose code is `language_code`, or null.
const BraveOnlyLocale* FindBraveOnlyLocaleByCode(LanguageCode language_code) {
  auto it = std::ranges::find(kBraveOnlyLocales, language_code,
                              &BraveOnlyLocale::language_code);
  return it != kBraveOnlyLocales.end() ? &*it : nullptr;
}

// The SODA language config of the Brave-only locale whose name is
// `language_name`, or none. Only SODA's own install code reads the other
// fields, and Brave does not run it.
std::optional<SodaLanguagePackComponentConfig> GetBraveOnlyLanguageConfig(
    std::string_view language_name) {
  const BraveOnlyLocale* locale = FindBraveOnlyLocaleByName(language_name);
  if (!locale) {
    return std::nullopt;
  }
  return SodaLanguagePackComponentConfig{
      .language_code = locale->language_code,
      .language_name = locale->language_name,
  };
}

}  // namespace

}  // namespace speech

#include <components/soda/constants.cc>
