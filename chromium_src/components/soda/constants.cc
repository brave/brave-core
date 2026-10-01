/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/soda/constants.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/i18n/legacy_language_tag_helpers.h"
#include "base/strings/string_util.h"

namespace speech {

namespace {

constexpr std::string_view kEnglishLanguageSubtag = "en";

// Keep in sync with `MULTILINGUAL_SUPPORTED_LANGUAGES` in
// brave/components/local_ai/resources/speech_worker/configs.ts.
constexpr auto kMultilingualLanguages = std::to_array<std::string_view>({
    "es-ES",
    "es-US",
    "it-IT",
    "pt-BR",
    "pt-PT",
    "hi-IN",
});

}  // namespace

bool IsBraveOnDeviceSpeechLanguageSupported(std::string_view language) {
  if (base::i18n::GetLanguageSubtagUsingLanguageTag(language) ==
      kEnglishLanguageSubtag) {
    return true;
  }
  return std::ranges::any_of(
      kMultilingualLanguages, [language](std::string_view supported) {
        return base::EqualsCaseInsensitiveASCII(supported, language);
      });
}

std::vector<std::string> GetBraveOnDeviceSpeechSodaLanguageNames() {
  std::vector<std::string> names;
  auto add_pack_for = [&names](std::string_view language) {
    std::optional<SodaLanguagePackComponentConfig> config =
        GetLanguageComponentConfigMatchingLanguageSubtag(language);
    // Without a pack, SODA refuses the language before Brave is asked.
    CHECK(config) << language;
    std::string name(config->language_name);
    if (!std::ranges::contains(names, name)) {
      names.push_back(std::move(name));
    }
  };
  add_pack_for(kEnglishLanguageSubtag);
  for (std::string_view language : kMultilingualLanguages) {
    add_pack_for(language);
  }
  return names;
}

}  // namespace speech

#include <components/soda/constants.cc>
