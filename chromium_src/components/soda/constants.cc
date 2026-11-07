/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/soda/constants.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/i18n/language_tag.h"
#include "base/i18n/language_tag_matcher.h"
#include "base/i18n/tag_converters.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"

namespace speech {

namespace {

using Model = BraveOnDeviceSpeechModelKind;

// Each tag is a correct, canonical BCP 47 tag, even where the model card uses
// another label (the card writes Arabic as "ar-AR", which is Arabic in
// Argentina).
//
// Where a language has more than one locale, its default comes first. The
// matcher gives a tag such as "fr" or "fr-BE" the first of two locales at the
// same distance, so "fr-FR" must come before "fr-CA".
//
// Keep the multilingual tags in sync with
// `MULTILINGUAL_SUPPORTED_LANGUAGES` in
// brave/components/local_ai/resources/speech_worker/configs.ts.
constexpr auto kLocales = std::to_array<BraveOnDeviceSpeechLocale>({
    // The English model. It serves every English tag.
    {"en-US", LanguageCode::kEnUs, Model::kEnglish},

    // The multilingual model, transcription-ready locales.
    {"ar", LanguageCode::kAr, Model::kMultilingual},
    {"de-DE", LanguageCode::kDeDe, Model::kMultilingual},
    {"es-ES", LanguageCode::kEsEs, Model::kMultilingual},
    {"es-US", LanguageCode::kEsUs, Model::kMultilingual},
    {"fr-FR", LanguageCode::kFrFr, Model::kMultilingual},
    {"fr-CA", LanguageCode::kFrCa, Model::kMultilingual},
    {"hi-IN", LanguageCode::kHiIn, Model::kMultilingual},
    {"it-IT", LanguageCode::kItIt, Model::kMultilingual},
    {"ja-JP", LanguageCode::kJaJp, Model::kMultilingual},
    {"ko-KR", LanguageCode::kKoKr, Model::kMultilingual},
    {"nl-NL", LanguageCode::kNlNl, Model::kMultilingual},
    {"pt-BR", LanguageCode::kPtBr, Model::kMultilingual},
    {"pt-PT", LanguageCode::kPtPt, Model::kMultilingual},
    {"ru-RU", LanguageCode::kRuRu, Model::kMultilingual},
    {"tr-TR", LanguageCode::kTrTr, Model::kMultilingual},
    {"uk-UA", LanguageCode::kUkUa, Model::kMultilingual},
    {"vi-VN", LanguageCode::kViVn, Model::kMultilingual},

    // The multilingual model, broad-coverage locales.
    {"bg-BG", LanguageCode::kBgBg, Model::kMultilingual},
    {"cs-CZ", LanguageCode::kCsCz, Model::kMultilingual},
    {"da-DK", LanguageCode::kDaDk, Model::kMultilingual},
    {"et-EE", LanguageCode::kEtEe, Model::kMultilingual},
    {"fi-FI", LanguageCode::kFiFi, Model::kMultilingual},
    {"hr-HR", LanguageCode::kHrHr, Model::kMultilingual},
    {"hu-HU", LanguageCode::kHuHu, Model::kMultilingual},
    {"nb-NO", LanguageCode::kNbNo, Model::kMultilingual},
    {"pl-PL", LanguageCode::kPlPl, Model::kMultilingual},
    {"ro-RO", LanguageCode::kRoRo, Model::kMultilingual},
    {"sk-SK", LanguageCode::kSkSk, Model::kMultilingual},
    {"sv-SE", LanguageCode::kSvSe, Model::kMultilingual},
    {"zh-CN", LanguageCode::kZhCn, Model::kMultilingual},
});

// Languages that a locale above would match, and that no model serves.
// Nynorsk ("nn") and Bokmål ("nb") are both under "no", so without this the
// matcher gives Nynorsk the Bokmål locale, and the user gets Bokmål text. The
// model card does not mark Nynorsk ready.
constexpr auto kRefusedLanguageSubtags = std::to_array<std::string_view>({
    "nn",
});

// Matches a page's tag against the locales. Built once, because building it
// computes the distance from each locale to its parent locales.
const base::i18n::LanguageTagMatcher& GetLocaleMatcher() {
  static const base::NoDestructor<base::i18n::LanguageTagMatcher> matcher([] {
    std::vector<base::i18n::LanguageTag> tags;
    for (const BraveOnDeviceSpeechLocale& locale : kLocales) {
      std::optional<base::i18n::LanguageTag> tag =
          base::i18n::GetLanguageTagFromString(locale.locale);
      CHECK(tag) << locale.locale;
      tags.push_back(*std::move(tag));
    }
    return base::i18n::LanguageTagMatcher::Create(tags);
  }());
  return *matcher;
}

}  // namespace

base::span<const BraveOnDeviceSpeechLocale> GetBraveOnDeviceSpeechLocales() {
  return kLocales;
}

const BraveOnDeviceSpeechLocale* ResolveBraveOnDeviceSpeechLocale(
    std::string_view language) {
  std::optional<base::i18n::LanguageTag> tag =
      base::i18n::GetLanguageTagFromString(language);
  if (!tag ||
      std::ranges::contains(kRefusedLanguageSubtags, tag->language_subtag())) {
    return nullptr;
  }
  std::optional<base::i18n::LanguageTag> match = GetLocaleMatcher().Match(*tag);
  return match ? FindBraveOnDeviceSpeechLocaleByName(match->tag_string())
               : nullptr;
}

const BraveOnDeviceSpeechLocale* FindBraveOnDeviceSpeechLocaleByName(
    std::string_view locale) {
  auto it = std::ranges::find_if(
      kLocales, [locale](const BraveOnDeviceSpeechLocale& candidate) {
        return base::EqualsCaseInsensitiveASCII(candidate.locale, locale);
      });
  return it != kLocales.end() ? &*it : nullptr;
}

const BraveOnDeviceSpeechLocale* FindBraveOnDeviceSpeechLocaleByCode(
    LanguageCode language_code) {
  auto it = std::ranges::find(kLocales, language_code,
                              &BraveOnDeviceSpeechLocale::language_code);
  return it != kLocales.end() ? &*it : nullptr;
}

std::optional<SodaLanguagePackComponentConfig>
GetBraveOnDeviceSpeechLanguageConfig(std::string_view language) {
  const BraveOnDeviceSpeechLocale* locale =
      ResolveBraveOnDeviceSpeechLocale(language);
  if (!locale) {
    return std::nullopt;
  }
  // SODA's own config, where SODA has one for the code, keeps its pref names
  // and key. Only SODA's own install code reads them, and Brave does not run
  // it, so a config without them is safe too.
  std::optional<SodaLanguagePackComponentConfig> config =
      GetLanguageComponentConfig(locale->language_code);
  if (!config) {
    return SodaLanguagePackComponentConfig{
        .language_code = locale->language_code,
        .language_name = locale->locale,
    };
  }
  config->language_name = locale->locale;
  return config;
}

}  // namespace speech

#include <components/soda/constants.cc>
