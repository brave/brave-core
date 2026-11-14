/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_locales.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/i18n/language_tag.h"
#include "base/i18n/language_tag_matcher.h"
#include "base/i18n/tag_converters.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"

namespace local_ai {

namespace {

// Each tag must be a canonical BCP 47 tag that is in only one row. The
// matcher gives "fr" or "fr-BE" the first of two locales at the same
// distance, so "fr-FR" must come before "fr-CA". The English model serves only
// en-US, the one locale for all English tags. The multilingual model serves
// all other locales.
constexpr auto kLocales = std::to_array<OnDeviceSpeechLocale>({
    {"en-US", OnDeviceSpeechModel::kEnglish},
    {"ar", OnDeviceSpeechModel::kMultilingual},
    {"de-DE", OnDeviceSpeechModel::kMultilingual},
    {"es-ES", OnDeviceSpeechModel::kMultilingual},
    {"es-US", OnDeviceSpeechModel::kMultilingual},
    {"fr-FR", OnDeviceSpeechModel::kMultilingual},
    {"fr-CA", OnDeviceSpeechModel::kMultilingual},
    {"hi-IN", OnDeviceSpeechModel::kMultilingual},
    {"it-IT", OnDeviceSpeechModel::kMultilingual},
    {"ja-JP", OnDeviceSpeechModel::kMultilingual},
    {"ko-KR", OnDeviceSpeechModel::kMultilingual},
    {"nl-NL", OnDeviceSpeechModel::kMultilingual},
    {"pt-BR", OnDeviceSpeechModel::kMultilingual},
    {"pt-PT", OnDeviceSpeechModel::kMultilingual},
    {"ru-RU", OnDeviceSpeechModel::kMultilingual},
    {"tr-TR", OnDeviceSpeechModel::kMultilingual},
    {"uk-UA", OnDeviceSpeechModel::kMultilingual},
    {"vi-VN", OnDeviceSpeechModel::kMultilingual},
});

// Matches a page's tag against the locales. A region can select a locale that
// is not the first of its language, through its CLDR parent locale: "es-MX"
// gives "es-US" (both under es-419), and "pt-AO" gives "pt-PT". A match on the
// language alone gives es-ES and pt-BR. Built once, because building it
// computes the distance from each locale to its parent locales.
const base::i18n::LanguageTagMatcher& GetLocaleMatcher() {
  static const base::NoDestructor<base::i18n::LanguageTagMatcher> kMatcher([] {
    std::vector<base::i18n::LanguageTag> tags;
    for (const OnDeviceSpeechLocale& locale : kLocales) {
      // A tag that does not parse is left out, so it resolves to no locale.
      if (std::optional<base::i18n::LanguageTag> tag =
              base::i18n::GetLanguageTagFromString(locale.tag)) {
        tags.push_back(*std::move(tag));
      }
    }
    return base::i18n::LanguageTagMatcher::Create(tags);
  }());
  return *kMatcher;
}

// The locale whose tag is `tag`, ignoring case, or null.
const OnDeviceSpeechLocale* FindOnDeviceSpeechLocale(std::string_view tag) {
  auto it = std::ranges::find_if(
      kLocales, [tag](const OnDeviceSpeechLocale& candidate) {
        return base::EqualsCaseInsensitiveASCII(candidate.tag, tag);
      });
  return it != kLocales.end() ? &*it : nullptr;
}

}  // namespace

base::span<const OnDeviceSpeechLocale> GetOnDeviceSpeechLocales() {
  return kLocales;
}

const OnDeviceSpeechLocale* ResolveOnDeviceSpeechLocale(
    std::string_view language) {
  std::optional<base::i18n::LanguageTag> tag =
      base::i18n::GetLanguageTagFromString(language);
  if (!tag) {
    return nullptr;
  }
  std::optional<base::i18n::LanguageTag> match = GetLocaleMatcher().Match(*tag);
  return match ? FindOnDeviceSpeechLocale(match->tag_string()) : nullptr;
}

}  // namespace local_ai
