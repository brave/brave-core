/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TRANSLATE_TRANSLATE_BUBBLE_MODEL_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TRANSLATE_TRANSLATE_BUBBLE_MODEL_IMPL_H_

#define TranslateBubbleModelImpl ChromiumTranslateBubbleModelImpl
#include "../../../../../../../chrome/browser/ui/translate/translate_bubble_model_impl.h"
#undef TranslateBubbleModelImpl

class BraveLanguageMap;
class BraveTranslateBubbleModelImpl : public ChromiumTranslateBubbleModelImpl {
 public:
  BraveTranslateBubbleModelImpl(
      translate::TranslateStep step,
      std::unique_ptr<translate::TranslateUIDelegate> ui_delegate);

  ~BraveTranslateBubbleModelImpl() override;

  int GetNumberOfSourceLanguages() const override;
  int GetNumberOfTargetLanguages() const override;
  std::u16string GetSourceLanguageNameAt(int index) const override;
  std::u16string GetTargetLanguageNameAt(int index) const override;
  int GetSourceLanguageIndex() const override;
  int GetTargetLanguageIndex() const override;
  void UpdateSourceLanguageIndex(int index) override;
  void UpdateTargetLanguageIndex(int index) override;

 private:
  std::unique_ptr<BraveLanguageMap> source_language_map_, target_language_map_;
};

using TranslateBubbleModelImpl = BraveTranslateBubbleModelImpl;

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TRANSLATE_TRANSLATE_BUBBLE_MODEL_IMPL_H_
