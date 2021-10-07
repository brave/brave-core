/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/translate/core/browser/brave_translate_language_filter.h"

namespace translate {

bool IsSourceLanguageCodeSupported(const std::string& lang_code) {
  // Note: keep sync with language/language.go (brave/go-translate repo)
  return lang_code == "und" || lang_code == "en" || lang_code == "es" ||
         lang_code == "et" || lang_code == "it" || lang_code == "pt" ||
         lang_code == "ru";
}

bool IsTargetLanguageCodeSupported(const std::string& lang_code) {
  // Note: keep sync with language/language.go (brave/go-translate repo)
  return lang_code == "de" || lang_code == "en" || lang_code == "es" ||
         lang_code == "et" || lang_code == "ru";
}

}  // namespace translate
