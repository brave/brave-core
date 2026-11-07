/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SODA_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SODA_CONSTANTS_H_

#include <components/soda/constants.h>  // IWYU pragma: export

#include <optional>
#include <string_view>

#include "base/containers/span.h"

namespace speech {

// The Brave on-device model that transcribes a locale.
enum class BraveOnDeviceSpeechModelKind {
  kEnglish,
  kMultilingual,
};

// A locale that one of Brave's on-device models transcribes.
struct BraveOnDeviceSpeechLocale {
  // The canonical BCP 47 tag. `available()` and `install()` record it for each
  // origin, and the speech worker gets it as the language of a stream.
  std::string_view locale;
  // Unique among the locales. SODA's code when SODA has one, otherwise a code
  // that Brave adds to `LanguageCode`.
  LanguageCode language_code;
  BraveOnDeviceSpeechModelKind model;
};

// All the locales that Brave's on-device models transcribe: one English locale
// for every English tag, and one for each prompt of the multilingual model.
base::span<const BraveOnDeviceSpeechLocale> GetBraveOnDeviceSpeechLocales();

// The locale that serves `language`, a page's BCP 47 tag such as "de-AT", or
// null when no locale does. The match uses the language, script and region of
// the tag, so "de-AT" gives "de-DE", but "zh-TW" gives null.
const BraveOnDeviceSpeechLocale* ResolveBraveOnDeviceSpeechLocale(
    std::string_view language);

// The locale whose canonical tag is `locale`, ignoring case, or null.
const BraveOnDeviceSpeechLocale* FindBraveOnDeviceSpeechLocaleByName(
    std::string_view locale);

// The locale whose code is `language_code`, or null.
const BraveOnDeviceSpeechLocale* FindBraveOnDeviceSpeechLocaleByCode(
    LanguageCode language_code);

// The SODA language config that stands for the locale that serves `language`,
// or none when no locale does. Its name is the locale's canonical tag and its
// code is the locale's code.
std::optional<SodaLanguagePackComponentConfig>
GetBraveOnDeviceSpeechLanguageConfig(std::string_view language);

}  // namespace speech

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SODA_CONSTANTS_H_
