/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_language_list.h"

#define TranslateLanguageList TranslateLanguageList_ChromiumImpl
#include "src/components/translate/core/browser/translate_language_list.cc"
#undef TranslateLanguageList

namespace translate {

namespace {
const char* const kBraveDefaultLanguageList[] = {
    "de", "en", "es", "fr", "hi", "it", "ja",    "nl",
    "pl", "pt", "ro", "ru", "tr", "vi", "zh-CN",
};
}  // namespace

TranslateLanguageList::TranslateLanguageList() {
  supported_languages_ =
      std::vector<std::string>(std::begin(kBraveDefaultLanguageList),
                               std::end(kBraveDefaultLanguageList));
  DCHECK(
      std::is_sorted(supported_languages_.begin(), supported_languages_.end()));
  DCHECK(supported_languages_.end() ==
         std::adjacent_find(supported_languages_.begin(),
                            supported_languages_.end()));
}

}  // namespace translate
