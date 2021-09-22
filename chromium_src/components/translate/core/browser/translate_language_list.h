/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_LANGUAGE_LIST_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_LANGUAGE_LIST_H_

#define TranslateLanguageList ChromiumTranslateLanguageList
#include "../../../../../../../components/translate/core/browser/translate_language_list.h"
#undef TranslateLanguageList

#define TranslateLanguageList BraveTranslateLanguageList

namespace translate {

class BraveTranslateLanguageList : public ChromiumTranslateLanguageList {
 public:
  BraveTranslateLanguageList();
  ~BraveTranslateLanguageList() override;

  // Fills |languages| with the alphabetically sorted list of languages that the
  // translate server can translate to and from. May attempt a language list
  // request unless |translate_allowed| is false.
  void GetSupportedLanguages(bool translate_allowed,
                             std::vector<std::string>* languages);

  // Fills |languages| with the alphabetically sorted list of languages that the
  // translate server can translate from. May attempt a language list
  // request unless |translate_allowed| is false.
  void GetSupportedSourceLanguages(bool translate_allowed,
                                   std::vector<std::string>* languages);

  // Fills |languages| with the alphabetically sorted list of languages that the
  // translate server can translate to. May attempt a language list
  // request unless |translate_allowed| is false.
  void GetSupportedTargetLanguages(bool translate_allowed,
                                   std::vector<std::string>* languages);

  // Returns true if |language| is supported by the translation server
  // as source language.
  bool IsSupportedSourceLanguage(base::StringPiece language);

  // Returns true if |language| is supported by the translation server
  // as target language.
  bool IsSupportedTargetLanguage(base::StringPiece language);

  // Parses |language_list| containing the list of languages that the translate
  // server can translate to and from. Returns true iff the list is parsed
  // without syntax errors.
  bool SetSupportedLanguages(base::StringPiece language_list);

  void RequestLanguageList();

  // Callback function called when TranslateURLFetcher::Request() is finished.
  void OnLanguageListFetchComplete(bool success, const std::string& data);

  // static const values shared with our browser tests.
  static const char kSourceLanguagesKey[];

 private:
  // The languages supported by the translation server
  // as source ones, sorted alphabetically
  std::vector<std::string> supported_source_languages_;

  // The languages supported by the translation server
  // as target ones, sorted alphabetically
  std::vector<std::string> supported_target_languages_;

  DISALLOW_COPY_AND_ASSIGN(BraveTranslateLanguageList);
};

}  // namespace translate

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_LANGUAGE_LIST_H_
