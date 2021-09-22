/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_DOWNLOAD_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_DOWNLOAD_MANAGER_H_

#define TranslateDownloadManager ChromiumTranslateDownloadManager
#include "../../../../../../../components/translate/core/browser/translate_download_manager.h"
#undef TranslateDownloadManager

#define TranslateDownloadManager BraveTranslateDownloadManager

namespace translate {

// Manages the downloaded resources for Translate, such as the translate script
// and the language list.
class BraveTranslateDownloadManager : public ChromiumTranslateDownloadManager {
 public:
  // Returns the singleton instance.
  static BraveTranslateDownloadManager* GetInstance();

  // Fills |languages| with the alphabetically sorted list of languages that the
  // translate server can translate to and from. May cause a language list
  // request unless |translate_allowed| is false.
  static void GetSupportedLanguages(bool translate_allowed,
                                    std::vector<std::string>* languages);

  // Fills |languages| with the alphabetically sorted list of languages that the
  // translate server can translate from. May cause a language list
  // request unless |translate_allowed| is false.
  static void GetSupportedSourceLanguages(bool translate_allowed,
                                          std::vector<std::string>* languages);

  // Fills |languages| with the alphabetically sorted list of languages that the
  // translate server can translate to. May cause a language list
  // request unless |translate_allowed| is false.
  static void GetSupportedTargetLanguages(bool translate_allowed,
                                          std::vector<std::string>* languages);

  // Returns true if |language| is supported by the translation server as source language.
  static bool IsSupportedSourceLanguage(base::StringPiece language);

  // Returns true if |language| is supported by the translation server as target language.
  static bool IsSupportedTargetLanguage(base::StringPiece language);

  // Returns the last-updated time when Chrome received a language list from a
  // Translate server. Returns null time if Chrome hasn't received any lists.
  static base::Time GetSupportedLanguagesLastUpdated();

  // Returns the language code that can be used with the Translate method for a
  // specified |language|. (ex. GetLanguageCode("en-US") will return "en", and
  // GetLanguageCode("zh-CN") returns "zh-CN")
  static std::string GetLanguageCode(base::StringPiece language);

 private:
  friend struct base::DefaultSingletonTraits<BraveTranslateDownloadManager>;

  BraveTranslateDownloadManager();

  ~BraveTranslateDownloadManager() override;
};

}  // namespace translate

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_DOWNLOAD_MANAGER_H_
