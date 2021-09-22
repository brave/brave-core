/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_download_manager.h"

#undef TranslateDownloadManager
#define TranslateDownloadManager ChromiumTranslateDownloadManager
#include "../../../../../../../components/translate/core/browser/translate_download_manager.cc"
#undef TranslateDownloadManager
#define TranslateDownloadManager BraveTranslateDownloadManager

namespace translate {

// static
BraveTranslateDownloadManager* BraveTranslateDownloadManager::GetInstance() {
  return base::Singleton<BraveTranslateDownloadManager>::get();
}

BraveTranslateDownloadManager::BraveTranslateDownloadManager()
    : ChromiumTranslateDownloadManager() {}

BraveTranslateDownloadManager::~BraveTranslateDownloadManager() {}

// static
void TranslateDownloadManager::GetSupportedLanguages(
    bool translate_allowed,
    std::vector<std::string>* languages) {
  TranslateLanguageList* language_list = GetInstance()->language_list();
  DCHECK(language_list);

  language_list->GetSupportedLanguages(translate_allowed, languages);
}

// static
void BraveTranslateDownloadManager::GetSupportedSourceLanguages(
    bool translate_allowed,
    std::vector<std::string>* languages) {
  TranslateLanguageList* language_list = GetInstance()->language_list();
  DCHECK(language_list);

  language_list->GetSupportedSourceLanguages(translate_allowed, languages);
}

// static
void BraveTranslateDownloadManager::GetSupportedTargetLanguages(
    bool translate_allowed,
    std::vector<std::string>* languages) {
  TranslateLanguageList* language_list = GetInstance()->language_list();
  DCHECK(language_list);

  language_list->GetSupportedTargetLanguages(translate_allowed, languages);
}

// static
bool BraveTranslateDownloadManager::IsSupportedSourceLanguage(base::StringPiece language) {
  TranslateLanguageList* language_list = GetInstance()->language_list();
  DCHECK(language_list);

  return language_list->IsSupportedSourceLanguage(language);
}

// static
bool BraveTranslateDownloadManager::IsSupportedTargetLanguage(base::StringPiece language) {
  TranslateLanguageList* language_list = GetInstance()->language_list();
  DCHECK(language_list);

  return language_list->IsSupportedTargetLanguage(language);
}

// static
base::Time TranslateDownloadManager::GetSupportedLanguagesLastUpdated() {
  TranslateLanguageList* language_list = GetInstance()->language_list();
  DCHECK(language_list);

  return language_list->last_updated();
}

// static
std::string TranslateDownloadManager::GetLanguageCode(
    base::StringPiece language) {
  TranslateLanguageList* language_list = GetInstance()->language_list();
  DCHECK(language_list);

  return language_list->GetLanguageCode(language);
}

}  // namespace translate
