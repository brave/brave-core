/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/translate/core/browser/brave_translate_language_filter.h"

#include "base/containers/contains.h"
#include "base/strings/string_piece.h"

namespace translate {
namespace {
// Note: keep sync with language/language.go (brave/go-translate repo)
constexpr base::StringPiece kSourceLanguages[] = {"und", "en", "es", "et",
                                                  "it",  "pt", "ru"};

// Note: keep sync with language/language.go (brave/go-translate repo)
constexpr base::StringPiece kTargetLanguages[] = {"de", "en", "es", "et", "ru"};

}  // namespace

bool IsSourceLanguageCodeSupported(const std::string& lang_code) {
  return base::Contains(kSourceLanguages, lang_code);
}

bool IsTargetLanguageCodeSupported(const std::string& lang_code) {
  return base::Contains(kTargetLanguages, lang_code);
}

}  // namespace translate
