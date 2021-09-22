/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_UI_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_UI_DELEGATE_H_

#define TranslateUIDelegate ChromiumTranslateUIDelegate
#include "../../../../../../../components/translate/core/browser/translate_ui_delegate.h"
#undef TranslateUIDelegate

#define TranslateUIDelegate BraveTranslateUIDelegate

namespace translate {

class BraveTranslateUIDelegate : public ChromiumTranslateUIDelegate {
 public:
  BraveTranslateUIDelegate(const base::WeakPtr<TranslateManager>& translate_manager,
                           const std::string& source_language,
                           const std::string& target_language);

  ~BraveTranslateUIDelegate() override;

  // Returns the number of source languages supported.
  size_t GetNumberOfSourceLanguages() const;

  // Returns the number of target languages supported.
  size_t GetNumberOfTargetLanguages() const;

  // Returns the source language code.
  std::string GetSourceLanguageCode() const;

  // Updates the source language index.
  void UpdateSourceLanguageIndex(size_t language_index);

  void UpdateSourceLanguage(const std::string& language_code);

  // Returns the target language code.
  std::string GetTargetLanguageCode() const;

  // Updates the target language index.
  void UpdateTargetLanguageIndex(size_t language_index);

  void UpdateTargetLanguage(const std::string& language_code);

  // Returns the ISO code for the source language at |index|.
  std::string GetSourceLanguageCodeAt(size_t index) const;

  // Returns the ISO code for the target language at |index|.
  std::string GetTargetLanguageCodeAt(size_t index) const;

  // Returns the displayable name for the source language at |index|.
  std::u16string GetSourceLanguageNameAt(size_t index) const;

  // Returns the displayable name for the target language at |index|.
  std::u16string GetTargetLanguageNameAt(size_t index) const;

  // Starts translating the current page.
  void Translate();

 private:
  // The list supported source languages for translation.
  // The languages are sorted alphabetically based on the displayable name.
  std::vector<LanguageNamePair> source_languages_;

  // The list supported target languages for translation.
  // The languages are sorted alphabetically based on the displayable name.
  std::vector<LanguageNamePair> target_languages_;

  DISALLOW_COPY_AND_ASSIGN(BraveTranslateUIDelegate);
};

}  // namespace translate

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_UI_DELEGATE_H_
